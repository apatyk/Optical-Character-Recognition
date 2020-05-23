
/*
** Adam Patyk
** ECE 6310
** Lab 3: Letters
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

unsigned char *read_image(char **, int, int *, int *, int *);
unsigned char* threshold(unsigned char *, int, int, int);
void detect_letters(unsigned char *, unsigned char *, char *, int **, int, int, int, int, int);
unsigned char* thin(unsigned char *, int, int);
unsigned char* analyze_bp_ep(unsigned char *, int, int, int *, int *);
void analyze_thinning(int *, unsigned char*, int, int);

int main(int argc, char *argv[]) {
    FILE		    *fpt;
    unsigned char   *input_img, *template, *msf_img, *th_img, *thin_img, *bp_ep_img;
    char            tmp[2];
    int		        i, rows_img, cols_img, rows_templ, cols_templ, bytes, tmp1, tmp2, num_letters = 0;

    if (argc != 5) {
        printf("Usage: ./ocr [<input_file>.ppm] [<template_file.ppm] [ground_truth.txt] [msf_file.ppm]\n");
        exit(0);
    }

    // read input image
    input_img = read_image(argv, 1, &rows_img, &cols_img, &bytes);
    // read template image
    template = read_image(argv, 2, &rows_templ, &cols_templ, &bytes);
    // read msf image
    msf_img = read_image(argv, 4, &rows_img, &cols_img, &bytes);

    // read ground truth file
    if ((fpt = fopen(argv[3], "rb")) == NULL) {
        printf("Unable to open \"%s\" for reading\n", argv[3]);
        exit(0);
    }
    // determine length of file
    while (fscanf(fpt, "%c %d %d\n", tmp, &tmp1, &tmp2) != EOF) {
        num_letters++;
    }
    fseek(fpt, 0, SEEK_SET); // reset file pointer to beginning of file
    char *gt_letters = (char *)calloc(num_letters, sizeof(char));
    int **gt_coords = (int **)calloc(num_letters, sizeof(int *));
    for (i = 0; i < num_letters; i++) {
        gt_coords[i] = (int *)calloc(2, sizeof(int));
    }
    // read values into memory
    for (i = 0; i < num_letters; i++) {
        fscanf(fpt, "%c %d %d\n", &gt_letters[i], &gt_coords[i][0], &gt_coords[i][1]);
    }
    fclose(fpt);

    // check letter locations from ground truth to see if letters were detected
    detect_letters(input_img, msf_img, gt_letters, gt_coords, num_letters, rows_img, cols_img, rows_templ, cols_templ);

    // threshold image at T = 128 for demo purposes
    th_img = threshold(input_img, 128, rows_img, cols_img);
    // thin image for demo purposes
    thin_img = thin(th_img, rows_img, cols_img);
    // branchpoints & endpoints image for demo purposes
    bp_ep_img = analyze_bp_ep(thin_img, rows_img, cols_img, &tmp1, &tmp2);

    // write out thresholded image
    fpt = fopen("output_th.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", cols_img, rows_img);
    fwrite(th_img, cols_img * rows_img, 1, fpt);
    fclose(fpt);

    // write out thinned image
    fpt = fopen("output_thin.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", cols_img, rows_img);
    fwrite(thin_img, cols_img * rows_img, 1, fpt);
    fclose(fpt);

    // write out bp & ep image
    fpt = fopen("output_bp_ep.ppm", "w");
    fprintf(fpt, "P5 %d %d 255\n", cols_img, rows_img);
    fwrite(bp_ep_img, cols_img * rows_img, 1, fpt);

    // cleanup
    fclose(fpt);
    free(input_img);
    free(template);
    free(msf_img);
    free(th_img);
    free(thin_img);
    free(bp_ep_img);
    free(gt_letters);
    for (i = 0; i < num_letters; i++) {
        free(gt_coords[i]);
    }
    free(gt_coords);

    return 0;
}

// READ IMAGE
unsigned char *read_image(char **argv, int arg, int *rows, int *cols, int *bytes) {
    FILE *fpt;
    char header[320];

    if ((fpt = fopen(argv[arg], "rb")) == NULL) {
        printf("Unable to open \"%s\" for reading\n", argv[arg]);
        exit(0);
    }
    fscanf(fpt, "%s %d %d %d", header, cols, rows, bytes);
    if (strcmp(header,"P5") != 0  ||  *bytes != 255) {
        printf("Not a grayscale 8-bit PPM image\n");
        exit(0);
    }
    unsigned char *img = (unsigned char *)calloc(*rows * *cols, sizeof(unsigned char));
    header[0] = fgetc(fpt);	// read white-space character that separates header
    fread(img, 1, *cols * *rows, fpt);
    fclose(fpt);

    return img;
}

// THRESHOLD
unsigned char* threshold(unsigned char *input, int T, int rows, int cols) {
    int r, c;
    unsigned char *output = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));

    // threshold the image at the given threshold, T
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            output[r * cols + c] = (input[r * cols + c] > T) ? 255 : 0;
        }
    }

    return output;
}

// DETECT LETTERS
void detect_letters(unsigned char *orig_img, unsigned char *input, char *gt_text,
                    int **gt_coords, int num_letters, int rows_img, int cols_img,
                    int rows_templ, int cols_templ) {
    int i, j, T, r, c, TP, FP, detected;
    unsigned char *threshold_img;
    unsigned char *letter_window = (unsigned char *)calloc(rows_templ * cols_templ, sizeof(unsigned char));
    FILE *roc_ptr = fopen("ROC_data.txt", "w");

    for (T = 0; T < 256; T++) {
        TP = FP = 0;
        // threshold the image at the given threshold, T
        threshold_img = threshold(input, T, rows_img, cols_img);

        for (i = 0; i < num_letters; i++) {
            detected = 0;
            // check window around ground truth locations for matches
            for (r = gt_coords[i][1] - (rows_templ / 2); r <= gt_coords[i][1] + (rows_templ / 2); r++) {
                for (c = gt_coords[i][0] - (cols_templ / 2); c <= gt_coords[i][0] + (cols_templ / 2); c++) {
                    if (threshold_img[r * cols_img + c] == 255 && detected == 0)
                        detected = 1;
                }
            }

            // analyze letters with thinning
            if (detected == 1) {
                j = 0;
                for (r = gt_coords[i][1] - (rows_templ / 2); r <= gt_coords[i][1] + (rows_templ / 2); r++) {
                    for (c = gt_coords[i][0] - (cols_templ / 2); c <= gt_coords[i][0] + (cols_templ / 2); c++, j++) {
                        letter_window[j] = orig_img[r * cols_img + c];
                    }
                }
                analyze_thinning(&detected, letter_window, rows_templ, cols_templ);
            }
            if (detected == 1) {
                if (gt_text[i] == 'e') TP++; // iterate true positive count
                else FP++; // iterate false positive count
            }
        }
        // output TP & FP for each T to file
        fprintf(roc_ptr, "%d\t%d\t%d\n", T, TP, FP);
    }
}

// ANALYZE THINNING
void analyze_thinning(int *status, unsigned char *img, int rows, int cols) {
    int bp, ep;
    unsigned char *img_thr, *img_thin;

    // threshold image at 128
    img_thr = threshold(img, 128, rows, cols);

    // thin image down to a single pixel
    img_thin = thin(img_thr, rows, cols);

    // analyze the thinned image for branchpoints & endpoints
    analyze_bp_ep(img_thin, rows, cols, &bp,&ep);

    // mark letters as undetected if the branchpoints & endpoints don't match
    if (bp != 1 && ep != 1) *status = 0;

    free(img_thr);
    free(img_thin);
}

// THIN
unsigned char* thin(unsigned char *img, int rows, int cols) {
    int r, c, r2, c2, edge_non_edge_tr, num_edge_neighbors, pass_test;
    int to_erase = 1, erased = 0;
    unsigned char *output = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));
    bool *marked = (bool *)calloc(rows * cols, sizeof(bool));

    // copy input image into new image
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            output[r * cols + c] = img[r * cols + c];
        }
    }

    // loop until no pixels can be erased
    // make sure erased pixels doesn't exceed number in image
    while (to_erase > 0 && erased < rows * cols) {
        to_erase = 0;
        // scan for pixels to erase
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                // check only edge pixels (black)
                if (output[r * cols + c] == 0) {
                    edge_non_edge_tr = num_edge_neighbors = pass_test = 0;
                    // count edge-non-edge transitions (CW)
                    if (output[(r - 1) * cols + (c - 1)] == 0 && output[(r - 1) * cols + c] == 255)
                        edge_non_edge_tr++;
                    if (output[(r - 1) * cols + (c)] == 0 && output[(r - 1) * cols + (c + 1)] == 255)
                        edge_non_edge_tr++;
                    if (output[(r - 1) * cols + (c + 1)] == 0 && output[r * cols + (c + 1)] == 255)
                        edge_non_edge_tr++;
                    if (output[r * cols + (c + 1)] == 0 && output[(r + 1) * cols + (c + 1)] == 255)
                        edge_non_edge_tr++;
                    if (output[(r + 1) * cols + (c + 1)] == 0 && output[(r + 1) * cols + c] == 255)
                        edge_non_edge_tr++;
                    if (output[(r + 1) * cols + c] == 0 && output[(r + 1) * cols + (c - 1)] == 255)
                        edge_non_edge_tr++;
                    if (output[(r + 1) * cols + (c - 1)] == 0 && output[r * cols + (c - 1)] == 255)
                        edge_non_edge_tr++;
                    if (output[r * cols + (c - 1)] == 0 && output[(r - 1) * cols + (c - 1)] == 255)
                        edge_non_edge_tr++;
                    // count edge neighbors
                    for (r2 = -1; r2 <= 1; r2++) {
                        for (c2 = -1; c2 <= 1; c2++) {
                            if (output[(r + r2) * cols + (c + c2)] == 0) num_edge_neighbors++;
                        }
                    }
                    num_edge_neighbors--; // don't count center
                    // check edge condition
                    if (output[(r - 1) * cols + c] != 0 || output[r * cols + (c + 1)] != 0 ||
                            (output[r * cols + (c - 1)] != 0 && output[(r + 1) * cols + c] != 0)) {
                        pass_test = 1;
                    }
                    // mark pixels for erasure
                    if (edge_non_edge_tr == 1 && num_edge_neighbors >= 3 &&
                            num_edge_neighbors <= 7 && pass_test == 1) {
                        marked[r * cols + c] = 1;
                        to_erase++;
                        erased++;
                    }
                }
            }
        }
        // erase pixels
        for (r = 0; r < rows; r++) {
            for (c = 0; c < cols; c++) {
                if (marked[r * cols + c] == 1) output[r * cols + c] = 255;
            }
        }
    }

    return output;
}

// ANALYZE BRANCHPOINTS & ENDPOINTS
unsigned char* analyze_bp_ep(unsigned char *img, int rows, int cols, int *bp, int *ep) {
    int r, c, edge_non_edge_tr;

    unsigned char *output = (unsigned char *)calloc(rows * cols, sizeof(unsigned char));

    *bp = *ep = 0;

    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            edge_non_edge_tr = 0;
            // check only edge pixels (black)
            if (img[r * cols + c] == 0) {
                // count edge-non-edge transitions (CW)
                if (img[(r - 1) * cols + (c - 1)] == 0 && img[(r - 1) * cols + c] == 255)
                    edge_non_edge_tr++;
                if (img[(r - 1) * cols + (c)] == 0 && img[(r - 1) * cols + (c + 1)] == 255)
                    edge_non_edge_tr++;
                if (img[(r - 1) * cols + (c + 1)] == 0 && img[r * cols + (c + 1)] == 255)
                    edge_non_edge_tr++;
                if (img[r * cols + (c + 1)] == 0 && img[(r + 1) * cols + (c + 1)] == 255)
                    edge_non_edge_tr++;
                if (img[(r + 1) * cols + (c + 1)] == 0 && img[(r + 1) * cols + c] == 255)
                    edge_non_edge_tr++;
                if (img[(r + 1) * cols + c] == 0 && img[(r + 1) * cols + (c - 1)] == 255)
                    edge_non_edge_tr++;
                if (img[(r + 1) * cols + (c - 1)] == 0 && img[r * cols + (c - 1)] == 255)
                    edge_non_edge_tr++;
                if (img[r * cols + (c - 1)] == 0 && img[(r - 1)* cols + (c - 1)] == 255)
                    edge_non_edge_tr++;

                // check if endpoint
                if (edge_non_edge_tr == 1) {
                    (*ep)++;
                    output[r * cols + c] = 255;
                }
                // check if branchpoint
                if (edge_non_edge_tr > 2) {
                    (*bp)++;
                    output[r * cols + c] = 128;
                }
            }
        }
    }
    return output;
}