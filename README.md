# quirewise-pipeline

Application-level conversion and validation route composition for Quirewise.

`Quirewise::Pipeline` (CMake package `QuirewisePipeline`) is an independent,
MIT-licensed C11 library. It is a composition layer: it owns no format reader or
writer and depends on the installed Quirewise packages. It exports
`qw_pipeline_convert_file`, `qw_pipeline_convert_file_to_format`,
`qw_pipeline_check_file` and `qw_pipeline_version`.

## What it does

- Converts one file to PDF by running the registered document route first and,
  if it declines, the fallback routes the CLI needs: page containers (DjVu,
  PDG/PDZ, UVZ, CAJ, CEB, XPS), subtitles, comic archives including stored CBR
  and CB7, structured data, CAD, SVG and — for checks — audio packet scanning.
- Converts PDF input to TXT/HTML/Markdown, PNG/JPEG/BMP/TIFF/WebP (first page,
  144 dpi) and DOCX/EPUB (120 dpi page images, up to 24 pages).
- Converts one file to any other supported target with
  `qw_pipeline_convert_file_to_format`: JSON, JSON Lines, YAML, TOML, vCard,
  XML, SRT, WebVTT, LRC, MicroDVD, ASS, SSA, SAMI, SCC, XLSX, ODS, CSV, TSV,
  HTML, Markdown, DOCX, EPUB, HTMLZ, TXTZ, PMLZ and HWPX. The data, subtitle and
  worksheet targets are handed to the package that owns the writer (`data`,
  `subtitle`, `spreadsheet`); HTML and Markdown go through the document package.
  JSON↔YAML, SRT↔VTT and layout→HTML→Markdown are real conversions rather than
  PDF round trips. Decodable images can be re-encoded to the image package's
  supported formats.
- Checks one file through the same routes without writing output; fallbacks only
  run when the document route declines.
- Carries the application options that affect routing: raster page mode, PDZ
  page selection, archive password and audio track.
- Never overwrites an existing output path.

The package deliberately adds no second copy of any reader. `Quirewise::Document`
owns the registered format routes; this library only decides which installed
route handles the file and reports the most specific error it saw.

## Requirements

- C11 compiler and CMake 3.21 or newer.
- Installed Quirewise Runtime, Image, Page, PDG, Document, Data, Comic, Subtitle,
  Vector and Audio packages, found through `CMAKE_PREFIX_PATH`.
- CTest for the test suite.

## Build, test and install

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/quirewise/install
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /path/to/quirewise/install
```

Shared builds are available with `-DQUIREWISE_BUILD_SHARED=ON`.

## Use it from CMake

```cmake
find_package(QuirewisePipeline CONFIG REQUIRED)
target_link_libraries(your_app PRIVATE Quirewise::Pipeline)
```

`QuirewisePipelineConfig.cmake` resolves its package dependencies, so the
prefix must contain every package listed under Requirements.

## Quick start

Convert an HTML file to PDF, then validate it:

```c
#include <quirewise/pipeline.h>
#include <quirewise/status.h>

#include <stdio.h>
#include <string.h>

int main(void)
{
    QwPipelineOptions options = QW_PIPELINE_OPTIONS_INIT;
    QwConvertReport report;
    int status;

    memset(&report, 0, sizeof(report));
    status = qw_pipeline_convert_file("input.html", "output.pdf",
                                       QW_FORMAT_PDF, &options, &report);
    if (status != QW_STATUS_OK) {
        fprintf(stderr, "convert failed (%d): %s\n", status, report.note);
        return 1;
    }
    printf("%zu page(s), %zu bytes\n", report.pages_written, report.output_size);

    memset(&report, 0, sizeof(report));
    status = qw_pipeline_check_file("input.html", &options, &report);
    if (status != QW_STATUS_OK) {
        fprintf(stderr, "check failed (%d): %s\n", status, report.note);
        return 1;
    }
    puts("OK");
    return 0;
}
```

`output.pdf` must not exist before the call; the pipeline never overwrites.

## API overview

| Header | Purpose |
|---|---|
| `quirewise/pipeline.h` | composition entry points and options; includes `quirewise/convert.h` and therefore `quirewise/status.h` |

| Task | Functions |
|---|---|
| Version | `qw_pipeline_version` |
| Convert | `qw_pipeline_convert_file`, `qw_pipeline_convert_file_to_format` |
| Check | `qw_pipeline_check_file` |

`QwPipelineOptions` fields: `raster_page_mode`, `pdz_page_rule`,
`archive_password`, `audio_track`. Initialize with `QW_PIPELINE_OPTIONS_INIT`.

## Behavior and limits

- Paths, options and the archive password are borrowed for the call; the
  library retains no pointers after returning.
- `qw_pipeline_convert_file` accepts only `QW_FORMAT_PDF`, `QW_FORMAT_TXT` and
  `QW_FORMAT_PNG`. Any other target returns `QW_STATUS_ERR_INVALID_ARGUMENT`.
  `qw_pipeline_convert_file_to_format` accepts those three plus the family
  targets listed above; a target outside the supported set returns
  `QW_STATUS_ERR_UNSUPPORTED`.
- `QW_FORMAT_HTML` and `QW_FORMAT_MARKDOWN` use the document package's layout
  intermediate: the reader's HTML is returned as-is or converted with
  `quirewise-text`'s writer. PDF, image and subtitle inputs have no layout HTML.
- `QW_FORMAT_HTML` and `QW_FORMAT_MARKDOWN` use the document package's layout
  intermediate: the reader's HTML is returned as-is or converted with
  `quirewise-text`'s writer. PDF, image and subtitle inputs have no layout HTML.
- Family targets are parsed by the owning package using the input extension
  (and, from memory, an unambiguous content sniff), then written by that
  package's writer. A file from another family is refused with a
  family-specific note; it is not silently treated as text.
- Fallback routes add PDF only. For TXT, the only fallback is the CAD text
  inventory; PNG is handled by the document route alone.
- Convert order: document route → page containers → subtitles → comics →
  structured data → CAD → SVG. Check order: document route → page → subtitle →
  comic → data → CAD → SVG → audio.
- A failed conversion reports the first concrete route error, not the generic
  "no renderer" status. `QW_STATUS_ERR_UNSUPPORTED` means no route accepted the
  file.
- Existing output paths are never overwritten; the call returns
  `QW_STATUS_ERR_IO` and sets `report.note`. Fallback-route writes remove the
  partial file if writing fails.
- `qw_pipeline_check_file` discards generated bytes. A page container that is
  only partially readable is reported as `QW_STATUS_ERR_UNSUPPORTED`, not
  success.
- Paths are UTF-8; on Windows the library opens them with `_wfopen`.
- `pdz_page_rule`: 0 auto, 1 body, 2 body plus attachments, 3 all.
- `raster_page_mode` is `QW_RASTER_PAGE_MODE_A4` or
  `QW_RASTER_PAGE_MODE_IMAGE`; the pipeline default is `IMAGE`, unlike
  `QwConvertOptions`.
- `audio_track` is one-based for checks; 0 selects the default track.
- The pipeline does not modify PDG; PDG is consumed as installed.

## Testing

`ctest` runs `quirewise_pipeline_routes`, which converts HTML to PDF, verifies
the `%PDF-` header, asserts that an existing output is not overwritten, checks
the HTML, converts an SRT subtitle through the fallback chain, converts SRT to
WebVTT through the subtitle writer, and rejects a junk file with
`QW_STATUS_ERR_UNSUPPORTED`.

## Documentation

See [docs/usage.md](docs/usage.md) for route-order details, options, report
fields and error handling recipes.

## License

MIT. See [LICENSE](LICENSE).
