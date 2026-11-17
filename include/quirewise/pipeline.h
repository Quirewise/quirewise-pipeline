#ifndef QUIREWISE_PIPELINE_H
#define QUIREWISE_PIPELINE_H

#include <quirewise/convert.h>

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Application-level conversion routes.
 *
 * The document package owns the registered format routes; this library owns
 * the fallback chain the CLI used to implement itself (page archives, comics,
 * subtitles, structured data, CAD, SVG and audio checks). It is intentionally
 * a composition layer: it depends on the installed Quirewise packages and does
 * not add a second copy of any reader.
 */

typedef struct QwPipelineOptions {
    /* QW_RASTER_PAGE_MODE_A4 or QW_RASTER_PAGE_MODE_IMAGE. */
    int raster_page_mode;
    /* PDZ page rule: 0 auto, 1 body, 2 body+att, 3 all. */
    int pdz_page_rule;
    /* Optional CBZ/CBT/7z archive password. */
    const char *archive_password;
    /* Optional user password for encrypted PDF input. */
    const char *pdf_password;
    /* One-based audio track for check; 0 selects the default. */
    size_t audio_track;
    /* Optional companion MDD resource file for MDX dictionaries. */
    const char *mdd_path;
    /* Optional TrueType font embedded in PDF output (NULL keeps the
     * predefined Helvetica/STSong-Light pair). */
    const char *pdf_font_path;
    /* 1 = PDF -> DOCX/EPUB emits editable text paragraphs instead of
     * one page image per page. */
    int pdf_office_reflow;
    /* QwPdfaLevel; QW_PDFA_1B requires an embedded font. */
    int pdfa_level;
    /* Raster output edits applied by the image route. */
    unsigned image_crop_left;
    unsigned image_crop_top;
    unsigned image_crop_right;
    unsigned image_crop_bottom;
    unsigned image_resize_width;
    unsigned image_resize_height;
    unsigned sheet_index;
} QwPipelineOptions;

#define QW_PIPELINE_OPTIONS_INIT \
    { QW_RASTER_PAGE_MODE_IMAGE, 0, NULL, NULL, 0u, NULL, 0, 0, 0, 0, 0, 0, 0 }

/*! Version of the composition library, for diagnostics. */
QW_API const char *qw_pipeline_version(void);

/*!
 * Convert one file through the document route, then the fallback routes.
 * output_path is never overwritten. target is PDF, TXT or PNG; the fallback
 * routes only add PDF (and CAD text for TXT). On failure report carries the
 * most specific route error available.
 */
QW_API int qw_pipeline_convert_file(const char *input_path,
                                    const char *output_path,
                                    QwOutputFormat target,
                                    const QwPipelineOptions *options,
                                    QwConvertReport *report);

/*!
 * Read/validate one file through the same route chain without writing an
 * output. A partially readable page container is reported as UNSUPPORTED,
 * matching the CLI --check contract.
 */
QW_API int qw_pipeline_check_file(const char *input_path,
                                  const QwPipelineOptions *options,
                                  QwConvertReport *report);

/*!
 * Convert one file to any target the composition supports. PDF, TXT and PNG
 * use the full route chain above. Other targets (JSON, JSON Lines, YAML,
 * TOML, vCard, XML, SRT, WebVTT, LRC, MicroDVD, ASS, SSA, SAMI, SCC, CSV and
 * TSV) are routed to the package that owns the writer, using the input
 * extension and content to pick the parser. The output extension is not
 * validated here; callers that follow the CLI contract check it themselves.
 */
QW_API int qw_pipeline_convert_file_to_format(const char *input_path,
                                              const char *output_path,
                                              QwOutputFormat target,
                                              const QwPipelineOptions *options,
                                              QwConvertReport *report);

#ifdef __cplusplus
}
#endif

#endif
