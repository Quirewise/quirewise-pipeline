# quirewise-pipeline

Application-level conversion route composition for Quirewise.

CMake target: `Quirewise::Pipeline`. Public header: `quirewise/pipeline.h`.

## Capabilities

- `qw_pipeline_convert_file` runs the registered document route first, then the
  fallback routes the CLI needs: page containers (DjVu, PDG/PDZ, UVZ, CAJ,
  CEB), subtitles, comic archives, structured data, CAD, SVG and audio checks.
- `qw_pipeline_check_file` runs the same chain without writing output; a
  partially readable page container is reported as `UNSUPPORTED`.
- `qw_pipeline_version` reports the composition library version.

The library owns no reader or writer of its own: it composes the installed
Quirewise packages. The CLI keeps argument parsing, output naming and error
presentation; it calls this package for the conversion/check chain.

## Dependencies

Runtime, Image, Page, PDG, Document, Data, Comic, Subtitle, Vector and Audio.
Document and PDG bring their own transitive dependencies through their CMake
configs.

## Limits

- The fallback routes add PDF output; `TXT` is only extended with the CAD text
  inventory and `PNG` stays with the document raster route.
- Existing output paths are never overwritten; a failed write removes the
  partial file.
- This package does not modify or extend PDG; PDG is consumed as installed.

## Build

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=<install-prefix>
cmake --build build
ctest --test-dir build
```
