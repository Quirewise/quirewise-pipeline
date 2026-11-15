#include <quirewise/pipeline.h>

#include <quirewise/status.h>

#include "audio_check.h"
#include "cad_routes.h"
#include "comic_routes.h"
#include "data_routes.h"
#include "page_routes.h"
#include "pipeline_file.h"
#include "subtitle_routes.h"
#include "svg_routes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QW_PIPELINE_VERSION_STRING "0.1.0"

const char *qw_pipeline_version(void)
{
    return QW_PIPELINE_VERSION_STRING;
}

static int pipeline_has_extension(const char *path, const char *extension)
{
    const char *dot;
    size_t index = 0u;
    if (!path || !extension) return 0;
    dot = strrchr(path, '.');
    if (!dot || !dot[1] || !extension[0]) return 0;
    ++dot;
    while (dot[index] && extension[index]) {
        char a = dot[index];
        char b = extension[index];
        if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
        if (a != b) return 0;
        ++index;
    }
    return dot[index] == '\0' && extension[index] == '\0';
}

static int pipeline_path_exists(const char *path)
{
    FILE *file = qw_pipeline_fopen_utf8(path, "rb");
    if (!file) return 0;
    fclose(file);
    return 1;
}

static int pipeline_write_file(const char *path, const uint8_t *data,
                               size_t size, QwConvertReport *report)
{
    FILE *file;
    if (!path) return QW_STATUS_ERR_INVALID_ARGUMENT;
    if (pipeline_path_exists(path)) {
        if (report) snprintf(report->note, sizeof(report->note),
                             "output already exists and is never overwritten");
        return QW_STATUS_ERR_IO;
    }
    file = qw_pipeline_fopen_utf8(path, "wb");
    if (!file) {
        if (report) snprintf(report->note, sizeof(report->note),
                             "cannot create output file");
        return QW_STATUS_ERR_IO;
    }
    if (size > 0u && fwrite(data, 1u, size, file) != size) {
        fclose(file);
        remove(path);
        if (report) snprintf(report->note, sizeof(report->note),
                             "short write to output file");
        return QW_STATUS_ERR_IO;
    }
    if (fclose(file) != 0) {
        remove(path);
        return QW_STATUS_ERR_IO;
    }
    return QW_STATUS_OK;
}

/* Keep the first concrete route failure instead of the generic document
 * "no renderer" status. UNSUPPORTED is the routes' normal decline. */
static void pipeline_remember_route_error(int rc,
                                          const QwConvertReport *route_report,
                                          int *route_status,
                                          QwConvertReport *saved_report)
{
    if (rc == QW_STATUS_OK || rc == QW_STATUS_ERR_UNSUPPORTED) return;
    if (*route_status != QW_STATUS_OK) return;
    *route_status = rc;
    if (route_report) *saved_report = *route_report;
}

static void pipeline_convert_options(const QwPipelineOptions *options,
                                     QwConvertOptions *out)
{
    QwConvertOptions defaults = QW_CONVERT_OPTIONS_INIT;
    *out = defaults;
    if (!options) return;
    out->page_mode = options->raster_page_mode;
    out->zip_password = options->archive_password;
    out->pdf_password = options->pdf_password;
    out->audio_track = options->audio_track;
}

static int pipeline_check_routes(const char *input_path,
                                 const QwPipelineOptions *options,
                                 int status,
                                 QwConvertReport *report)
{
    int route_status = QW_STATUS_OK;
    QwConvertReport route_report;
    memset(&route_report, 0, sizeof(route_report));

    {
        QwConvertReport local;
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_page_convert_file(input_path, options, &pdf,
                                           &pdf_size, &local);
        qw_buffer_free(pdf);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_subtitle_convert_file(input_path, &pdf, &pdf_size,
                                               &local);
        qw_buffer_free(pdf);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_comic_convert_file(input_path, options, &pdf,
                                            &pdf_size, &local);
        qw_buffer_free(pdf);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_data_convert_file(input_path, &pdf, &pdf_size,
                                           &local);
        qw_buffer_free(pdf);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_cad_check_file(input_path, &local);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_svg_check_file(input_path, &local);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else if (rc == QW_STATUS_ERR_UNSUPPORTED &&
                   (pipeline_has_extension(input_path, "svg") ||
                    pipeline_has_extension(input_path, "svgz"))) {
            if (route_status == QW_STATUS_OK) {
                route_status = rc;
                route_report = local;
            }
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK) {
        QwConvertReport local;
        int rc;
        memset(&local, 0, sizeof(local));
        rc = qw_pipeline_audio_check_file(
            input_path, options ? options->audio_track : 0u, &local);
        if (rc == QW_STATUS_OK) {
            status = QW_STATUS_OK;
            if (report) *report = local;
        } else {
            pipeline_remember_route_error(rc, &local, &route_status,
                                          &route_report);
        }
    }

    if (status != QW_STATUS_OK && route_status != QW_STATUS_OK) {
        status = route_status;
        if (report) *report = route_report;
    }
    return status;
}

/* PDF, TXT and PNG own the composition route chain; every other target is a
 * family writer owned by the package that parses that family. */
int qw_pipeline_convert_file_to_format(const char *input_path,
                                       const char *output_path,
                                       QwOutputFormat target,
                                       const QwPipelineOptions *options,
                                       QwConvertReport *report)
{
    QwConvertOptions convert;

    if (!input_path || !output_path) return QW_STATUS_ERR_INVALID_ARGUMENT;
    if (target == QW_FORMAT_PDF || target == QW_FORMAT_TXT ||
        target == QW_FORMAT_PNG) {
        return qw_pipeline_convert_file(input_path, output_path, target,
                                        options, report);
    }
    pipeline_convert_options(options, &convert);
    return qw_convert_file_to_format(input_path, output_path, target, &convert,
                                     report);
}

int qw_pipeline_check_file(const char *input_path,
                           const QwPipelineOptions *options,
                           QwConvertReport *report)
{
    QwPipelineOptions defaults = QW_PIPELINE_OPTIONS_INIT;
    QwConvertOptions convert;
    QwConvertReport local;
    int status;

    if (!input_path) return QW_STATUS_ERR_INVALID_ARGUMENT;
    if (!options) options = &defaults;
    pipeline_convert_options(options, &convert);
    memset(&local, 0, sizeof(local));
    status = qw_check_file(input_path, &convert, &local);
    if (status == QW_STATUS_OK) {
        if (report) *report = local;
        return QW_STATUS_OK;
    }
    status = pipeline_check_routes(input_path, options, status, &local);
    if (status == QW_STATUS_OK && local.skipped_pages > 0u) {
        status = QW_STATUS_ERR_UNSUPPORTED;
        if (local.note[0] == '\0') {
            snprintf(local.note, sizeof(local.note),
                     "%zu page(s) could not be read", local.skipped_pages);
        }
    }
    if (report) *report = local;
    return status;
}

int qw_pipeline_convert_file(const char *input_path,
                             const char *output_path,
                             QwOutputFormat target,
                             const QwPipelineOptions *options,
                             QwConvertReport *report)
{
    QwPipelineOptions defaults = QW_PIPELINE_OPTIONS_INIT;
    QwConvertOptions convert;
    QwConvertReport local;
    QwConvertReport route_report;
    int status;
    int route_status = QW_STATUS_OK;

    if (!input_path || !output_path) return QW_STATUS_ERR_INVALID_ARGUMENT;
    if (target != QW_FORMAT_PDF && target != QW_FORMAT_TXT &&
        target != QW_FORMAT_PNG)
        return QW_STATUS_ERR_INVALID_ARGUMENT;
    if (!options) options = &defaults;
    pipeline_convert_options(options, &convert);
    memset(&local, 0, sizeof(local));
    memset(&route_report, 0, sizeof(route_report));

    status = qw_convert_file_to_format(input_path, output_path, target,
                                       &convert, &local);
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_page_convert_file(input_path, options, &pdf,
                                           &pdf_size, &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_subtitle_convert_file(input_path, &pdf, &pdf_size,
                                               &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_comic_convert_file(input_path, options, &pdf,
                                            &pdf_size, &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_data_convert_file(input_path, &pdf, &pdf_size,
                                           &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_cad_convert_file(input_path, options, &pdf, &pdf_size,
                                          &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_TXT) {
        uint8_t *text = NULL;
        size_t text_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_cad_extract_text(input_path, &text, &text_size,
                                          &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, text, text_size,
                                     &route_local);
            qw_buffer_free(text);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }
    if (status != QW_STATUS_OK && target == QW_FORMAT_PDF) {
        uint8_t *pdf = NULL;
        size_t pdf_size = 0u;
        QwConvertReport route_local;
        int rc;

        memset(&route_local, 0, sizeof(route_local));
        rc = qw_pipeline_svg_convert_file(input_path, options, &pdf, &pdf_size,
                                          &route_local);
        if (rc == QW_STATUS_OK) {
            rc = pipeline_write_file(output_path, pdf, pdf_size, &route_local);
            qw_buffer_free(pdf);
            if (rc == QW_STATUS_OK) {
                status = QW_STATUS_OK;
                local = route_local;
            } else {
                pipeline_remember_route_error(rc, &route_local, &route_status,
                                              &route_report);
            }
        } else if (rc == QW_STATUS_ERR_UNSUPPORTED &&
                   (pipeline_has_extension(input_path, "svg") ||
                    pipeline_has_extension(input_path, "svgz"))) {
            if (route_status == QW_STATUS_OK) {
                route_status = rc;
                route_report = route_local;
            }
        } else {
            pipeline_remember_route_error(rc, &route_local, &route_status,
                                          &route_report);
        }
    }

    if (status != QW_STATUS_OK && route_status != QW_STATUS_OK) {
        status = route_status;
        local = route_report;
    }
    if (report) *report = local;
    return status;
}
