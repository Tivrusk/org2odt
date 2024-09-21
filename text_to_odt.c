#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Function to check if the text is a heading
int is_heading(const char *text, int *heading_level) {
    if (text[0] == '#') {
        *heading_level = 1;
        for (int i = 1; i < strlen(text); i++) {
            if (text[i] == '#') {
                (*heading_level)++;
            } else {
                break;
            }
        }
        return 1;
    }
    return 0;
}

// Function to check if part of the text should be bold
int is_bold_part(const char *text) {
    return (strstr(text, "**") != NULL);
}

// Function to check if part of the text should be italic
int is_italic_part(const char *text) {
    return (strstr(text, "__") != NULL);
}

void create_odt(const char *text_file, const char *odt_file, const char *font_name) {
    FILE *fp_text, *fp_content, *fp_manifest, *fp_mimetype, *fp_styles;
    char buffer[1024];
    size_t n;
    int heading_level = 0;

    // Open the input text file
    fp_text = fopen(text_file, "r");
    if (fp_text == NULL) {
        perror("Error opening text file");
        exit(1);
    }

    // Create and write the 'mimetype' file (no newline at the end)
    fp_mimetype = fopen("mimetype", "w");
    if (fp_mimetype == NULL) {
        perror("Error creating mimetype file");
        exit(1);
    }
    fprintf(fp_mimetype, "application/vnd.oasis.opendocument.text");
    fclose(fp_mimetype);

    // Create and write the 'META-INF/manifest.xml' file
    system("mkdir -p META-INF");
    fp_manifest = fopen("META-INF/manifest.xml", "w");
    if (fp_manifest == NULL) {
        perror("Error creating manifest.xml file");
        exit(1);
    }
    fprintf(fp_manifest,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<manifest:manifest xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\">\n"
            "<manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.text\" manifest:full-path=\"/\"/>\n"
            "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>\n"
            "<manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"styles.xml\"/>\n"
            "</manifest:manifest>");
    fclose(fp_manifest);

    // Create and write the 'styles.xml' file to define the font, bold, italic, and heading styles
    fp_styles = fopen("styles.xml", "w");
    if (fp_styles == NULL) {
        perror("Error creating styles.xml file");
        exit(1);
    }
    fprintf(fp_styles,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<office:document-styles xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
            "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
            "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
            "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" office:version=\"1.2\">\n"
            "<office:styles>\n"
            // Define the normal font style
            "<style:style style:name=\"CustomFont\" style:family=\"text\">\n"
            "<style:text-properties fo:font-family=\"%s\" fo:font-size=\"11.5pt\"/>\n"
            "</style:style>\n"
            // Define the bold font style
            "<style:style style:name=\"BoldFont\" style:family=\"text\">\n"
            "<style:text-properties fo:font-family=\"%s\" fo:font-weight=\"bold\"/>\n"
            "</style:style>\n"
            // Define the italic font style
            "<style:style style:name=\"ItalicFont\" style:family=\"text\">\n"
            "<style:text-properties fo:font-family=\"%s\" fo:font-style=\"italic\"/>\n"
            "</style:style>\n"
            // Define heading styles (for simplicity, we're only using 3 heading levels)
            "<style:style style:name=\"Heading1\" style:family=\"paragraph\">\n"
            "<style:paragraph-properties fo:font-size=\"24pt\"/>\n"
            "</style:style>\n"
            "<style:style style:name=\"Heading2\" style:family=\"paragraph\">\n"
            "<style:paragraph-properties fo:font-size=\"18pt\"/>\n"
            "</style:style>\n"
            "<style:style style:name=\"Heading3\" style:family=\"paragraph\">\n"
            "<style:paragraph-properties fo:font-size=\"14pt\"/>\n"
            "</style:style>\n"
            "</office:styles>\n"
            "</office:document-styles>\n", font_name, font_name, font_name);
    fclose(fp_styles);

    // Create and write the 'content.xml' file
    fp_content = fopen("content.xml", "w");
    if (fp_content == NULL) {
        perror("Error creating content.xml file");
        exit(1);
    }

    // Write the XML structure
    fprintf(fp_content,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<office:document-content xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
            "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
            "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
            "office:version=\"1.2\">\n"
            "<office:body>\n"
            "<office:text>\n");

    // Read the text from the input file and write to content.xml
    while (fgets(buffer, sizeof(buffer), fp_text)) {
        // Check if the current line is a heading
        if (is_heading(buffer, &heading_level)) {
            // Remove the '#' characters and write heading
            char *heading_text = buffer + heading_level;
            fprintf(fp_content, "<text:h text:style-name=\"Heading%d\">%s</text:h>\n", heading_level, heading_text);
        } else {
            // Start a paragraph
            fprintf(fp_content, "<text:p>\n");

            // Split and process bold and italic parts within a line
            char *cursor = buffer;
            while (*cursor) {
                if (is_bold_part(cursor)) {
                    // Handle bold text
                    char *bold_start = strstr(cursor, "**");
                    if (bold_start) {
                        *bold_start = '\0';  // Split before '**'
                        fprintf(fp_content, "<text:span text:style-name=\"CustomFont\">%s</text:span>", cursor);
                        cursor = bold_start + 2;

                        char *bold_end = strstr(cursor, "**");
                        if (bold_end) {
                            *bold_end = '\0';  // Split after '**'
                            fprintf(fp_content, "<text:span text:style-name=\"BoldFont\">%s</text:span>", cursor);
                            cursor = bold_end + 2;
                        }
                    }
                } else if (is_italic_part(cursor)) {
                    // Handle italic text
                    char *italic_start = strstr(cursor, "__");
                    if (italic_start) {
                        *italic_start = '\0';  // Split before '__'
                        fprintf(fp_content, "<text:span text:style-name=\"CustomFont\">%s</text:span>", cursor);
                        cursor = italic_start + 2;

                        char *italic_end = strstr(cursor, "__");
                        if (italic_end) {
                            *italic_end = '\0';  // Split after '__'
                            fprintf(fp_content, "<text:span text:style-name=\"ItalicFont\">%s</text:span>", cursor);
                            cursor = italic_end + 2;
                        }
                    }
                } else {
                    // Write the normal text
                    fprintf(fp_content, "<text:span text:style-name=\"CustomFont\">%s</text:span>", cursor);
                    break;
                }
            }

            // End the paragraph
            fprintf(fp_content, "</text:p>\n");
        }
    }

    fprintf(fp_content, "</office:text>\n</office:body>\n</office:document-content>");
    fclose(fp_content);
    fclose(fp_text);

    // Create the ODT file by zipping the files
    char zip_command[256];
    snprintf(zip_command, sizeof(zip_command), "zip -r %s mimetype META-INF content.xml styles.xml", odt_file);
    system(zip_command);

    // Clean up temporary files and directories
    system("rm -rf mimetype META-INF content.xml styles.xml");

    printf("ODT file with font '%s' created successfully: %s\n", font_name, odt_file);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_text_file> <output_odt_file> <font_name>\n", argv[0]);
        return 1;
    }

    create_odt(argv[1], argv[2], argv[3]);

    return 0;
}
