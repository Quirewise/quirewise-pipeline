#include <quirewise/pipeline.h>
#include <quirewise/status.h>

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s at line %d\n", #condition, __LINE__); \
        return 1; \
    } \
} while (0)

static int write_file(const char *path, const void *data, size_t size)
{
    FILE *file = fopen(path, "wb");
    if (!file) return 0;
    if (size > 0u && fwrite(data, 1u, size, file) != size) {
        fclose(file);
        return 0;
    }
    return fclose(file) == 0;
}

int main(void)
{
    QwPipelineOptions options = QW_PIPELINE_OPTIONS_INIT;
    QwConvertReport report;
    static const char html[] =
        "<html><body><h1>Pipeline</h1><p>Route composition test.</p></body></html>";
    static const char srt[] =
        "1\n00:00:00,000 --> 00:00:02,000\nPipeline subtitle\n\n"
        "2\n00:00:02,500 --> 00:00:04,000\nSecond cue\n";
    static const char junk[] = "not a Quirewise input";

    remove("pipeline-test.html");
    remove("pipeline-test.pdf");
    remove("pipeline-test.srt");
    remove("pipeline-subtitle.pdf");
    remove("pipeline-junk.bin");
    remove("pipeline-junk.pdf");

    CHECK(qw_pipeline_version() != NULL && qw_pipeline_version()[0] != '\0');
    CHECK(write_file("pipeline-test.html", html, sizeof(html) - 1u));

    memset(&report, 0, sizeof(report));
    CHECK(qw_pipeline_convert_file("pipeline-test.html", "pipeline-test.pdf",
                                   QW_FORMAT_PDF, &options, &report) ==
          QW_STATUS_OK);
    {
        FILE *file = fopen("pipeline-test.pdf", "rb");
        char magic[5] = {0};
        CHECK(file != NULL);
        CHECK(fread(magic, 1u, 5u, file) == 5u);
        fclose(file);
        CHECK(memcmp(magic, "%PDF-", 5u) == 0);
    }
    /* The existing output must never be overwritten. */
    CHECK(qw_pipeline_convert_file("pipeline-test.html", "pipeline-test.pdf",
                                   QW_FORMAT_PDF, &options, &report) ==
          QW_STATUS_ERR_IO);
    CHECK(qw_pipeline_check_file("pipeline-test.html", &options, &report) ==
          QW_STATUS_OK);

    CHECK(write_file("pipeline-test.srt", srt, sizeof(srt) - 1u));
    memset(&report, 0, sizeof(report));
    CHECK(qw_pipeline_convert_file("pipeline-test.srt", "pipeline-subtitle.pdf",
                                   QW_FORMAT_PDF, &options, &report) ==
          QW_STATUS_OK);
    {
        FILE *file = fopen("pipeline-subtitle.pdf", "rb");
        char magic[5] = {0};
        CHECK(file != NULL);
        CHECK(fread(magic, 1u, 5u, file) == 5u);
        fclose(file);
        CHECK(memcmp(magic, "%PDF-", 5u) == 0);
    }
    CHECK(qw_pipeline_check_file("pipeline-test.srt", &options, &report) ==
          QW_STATUS_OK);

    CHECK(write_file("pipeline-junk.bin", junk, sizeof(junk) - 1u));
    CHECK(qw_pipeline_convert_file("pipeline-junk.bin", "pipeline-junk.pdf",
                                   QW_FORMAT_PDF, &options, &report) ==
          QW_STATUS_ERR_UNSUPPORTED);
    CHECK(qw_pipeline_check_file("pipeline-junk.bin", &options, &report) ==
          QW_STATUS_ERR_UNSUPPORTED);

    remove("pipeline-test.html");
    remove("pipeline-test.pdf");
    remove("pipeline-test.srt");
    remove("pipeline-subtitle.pdf");
    remove("pipeline-junk.bin");
    remove("pipeline-junk.pdf");
    puts("PASS: pipeline route composition (document, subtitle fallback, no overwrite, rejection)");
    return 0;
}
