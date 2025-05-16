#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "uart_touchscreen.h"

#define NUM_CALIBRATION_POINTS 5

typedef struct {
    double a, b, c;
    double d, e, f;
    // Last row is implicitly [0, 0, 1] for 2D transformations
} TransformationMatrix;

TransformationMatrix cal_matrix;

void collect_calibration_data(Pair raw_points[]);
TransformationMatrix calculate_calibration_matrix(const Pair screen_points[], const Pair raw_points[], int num_points);
Pair apply_calibration(const TransformationMatrix* matrix, const Pair* raw_point);
void print_matrix(const TransformationMatrix* matrix);
void calibrate();

#endif   