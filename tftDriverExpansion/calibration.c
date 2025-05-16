#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include "include/calibration.h"
#include "include/print.h"

// Known calibration points (normalized screen coordinates 0-1)
static const Pair calibration_screen_points[NUM_CALIBRATION_POINTS] = {
    {0.9 * X_SIZE, 0.1 * Y_SIZE},  // top-left
    {0.9 * X_SIZE, 0.9 * Y_SIZE},  // top-right
    {0.5 * X_SIZE, 0.5 * Y_SIZE},  // center
    {0.1 * X_SIZE, 0.1 * Y_SIZE},  // bottom-left
    {0.1 * X_SIZE, 0.9 * Y_SIZE}   // bottom-right
};

void calibrate() {

    Pair raw_touch_points[NUM_CALIBRATION_POINTS];
    // Collect raw touch data
    collect_calibration_data(raw_touch_points);
    
    // Calculate calibration matrix
    cal_matrix = calculate_calibration_matrix(
        calibration_screen_points, 
        raw_touch_points, 
        NUM_CALIBRATION_POINTS
    );
    
    /* DEBUG PURPOSE 
    printf("\nCalibration complete. Transformation matrix:\n");
    print_matrix(&cal_matrix);
    
    // Test calibration
    printf("\nTesting calibration - enter raw points to see calibrated result (enter x=-1 to exit)\n");
    while(1) {
        Pair test_point;
        printf("Enter raw X,Y: ");
        scanf("%lf %lf", &test_point.x, &test_point.y);
        
        if(test_point.x == -1) break;
        
        Pair calibrated = apply_calibration(&cal_matrix, &test_point);
        printf("Calibrated point: %.3f, %.3f\n", calibrated.x, calibrated.y);
    }*/

}

void collect_calibration_data(Pair raw_points[]) {
    calibrating = 1;
    for(int i = 0; i < NUM_CALIBRATION_POINTS; i++) {
        printArea(
            calibration_screen_points[i].x - 1, 
            calibration_screen_points[i].y - 1,
            calibration_screen_points[i].x + 1,
            calibration_screen_points[i].y + 1, 1);

        Pair pos = getxy();
        raw_points[i] = pos;
        printf("(%d, %d)\n", pos.x, pos.y);
        printf("Press c to continue, k to repeat\n");
        char c;
        scanf("%c", &c);
        while(c == '\n') scanf("%c", &c);
        while (getchar() != '\n');  // Clear input buffer
        if(c == 'k') --i;

        uint16_t aux_color = color;
        color = background_color;
        printArea(
            calibration_screen_points[i].x - 1, 
            calibration_screen_points[i].y - 1,
            calibration_screen_points[i].x + 1,
            calibration_screen_points[i].y + 1, 1);
        color = aux_color;
    }
    calibrating = 0;
}

TransformationMatrix calculate_calibration_matrix(const Pair screen_points[], const Pair raw_points[], int num_points) {
    // We're solving the system: A * params = B
    // For affine transformation: x' = a*x + b*y + c
    //                           y' = d*x + e*y + f
    
    // Allocate matrices
    double* A = (double*)malloc(2 * num_points * 6 * sizeof(double));
    double* B = (double*)malloc(2 * num_points * sizeof(double));
    
    // Fill matrices
    for(int i = 0; i < num_points; i++) {
        // For x equation
        A[12*i + 0] = raw_points[i].x;
        A[12*i + 1] = raw_points[i].y;
        A[12*i + 2] = 1;
        A[12*i + 3] = 0;
        A[12*i + 4] = 0;
        A[12*i + 5] = 0;
        B[2*i] = screen_points[i].x;
        
        // For y equation
        A[12*i + 6] = 0;
        A[12*i + 7] = 0;
        A[12*i + 8] = 0;
        A[12*i + 9] = raw_points[i].x;
        A[12*i + 10] = raw_points[i].y;
        A[12*i + 11] = 1;
        B[2*i + 1] = screen_points[i].y;
    }
    
    // Solve using least squares (simplified approach)
    TransformationMatrix matrix;
    
    // Solve for x parameters (a, b, c)
    double sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0, sum_y2 = 0;
    double sum_sx = 0, sum_sx_x = 0, sum_sx_y = 0;
    
    for(int i = 0; i < num_points; i++) {
        double x = raw_points[i].x;
        double y = raw_points[i].y;
        double sx = screen_points[i].x;
        
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_x2 += x * x;
        sum_y2 += y * y;
        sum_sx += sx;
        sum_sx_x += sx * x;
        sum_sx_y += sx * y;
    }
    
    // sum(x^2)*a + sum(xy)*b + sum(x)*c = sum(sx*x)
    // sum(xy)*a + sum(y^2)*b + sum(y)*c = sum(sx*y)
    // sum(x)*a + sum(y)*b + n*c = sum(sx)
    
    double det = sum_x2*(sum_y2*num_points - sum_y*sum_y) - 
                 sum_xy*(sum_xy*num_points - sum_y*sum_x) + 
                 sum_x*(sum_xy*sum_y - sum_y2*sum_x);
    
    if(fabs(det) < 1e-10) {
        printf("Error: Calibration points are colinear or poorly conditioned\n");
        exit(1);
    }
    
    matrix.a = (sum_sx_x*(sum_y2*num_points - sum_y*sum_y) - 
                sum_xy*(sum_sx_y*num_points - sum_sx*sum_y) + 
                sum_x*(sum_sx_y*sum_y - sum_y2*sum_sx)) / det;
    
    matrix.b = (sum_x2*(sum_sx_y*num_points - sum_sx*sum_y) - 
               sum_sx_x*(sum_xy*num_points - sum_x*sum_y) + 
               sum_x*(sum_xy*sum_sx - sum_sx_y*sum_x)) / det;
    
    matrix.c = (sum_x2*(sum_y2*sum_sx - sum_sx_y*sum_y) - 
                sum_xy*(sum_xy*sum_sx - sum_sx_y*sum_x) + 
                sum_sx_x*(sum_xy*sum_y - sum_y2*sum_x)) / det;
    
    // Now solve for y parameters (d, e, f)
    double sum_sy = 0, sum_sy_x = 0, sum_sy_y = 0;
    
    for(int i = 0; i < num_points; i++) {
        double x = raw_points[i].x;
        double y = raw_points[i].y;
        double sy = screen_points[i].y;
        
        sum_sy += sy;
        sum_sy_x += sy * x;
        sum_sy_y += sy * y;
    }
    
    matrix.d = (sum_sy_x*(sum_y2*num_points - sum_y*sum_y) - 
                sum_xy*(sum_sy_y*num_points - sum_sy*sum_y) + 
                sum_x*(sum_sy_y*sum_y - sum_y2*sum_sy)) / det;
    
    matrix.e = (sum_x2*(sum_sy_y*num_points - sum_sy*sum_y) - 
               sum_sy_x*(sum_xy*num_points - sum_x*sum_y) + 
               sum_x*(sum_xy*sum_sy - sum_sy_y*sum_x)) / det;
    
    matrix.f = (sum_x2*(sum_y2*sum_sy - sum_sy_y*sum_y) - 
                sum_xy*(sum_xy*sum_sy - sum_sy_y*sum_x) + 
                sum_sy_x*(sum_xy*sum_y - sum_y2*sum_x)) / det;
    
    free(A);
    free(B);
    
    return matrix;
}

Pair apply_calibration(const TransformationMatrix* matrix, const Pair* raw_point) {
    Pair calibrated;
    calibrated.x = matrix->a * raw_point->x + matrix->b * raw_point->y + matrix->c;
    calibrated.y = matrix->d * raw_point->x + matrix->e * raw_point->y + matrix->f;
    return calibrated;
}

void print_matrix(const TransformationMatrix* matrix) {
    printf("[%.6f, %.6f, %.6f]\n", matrix->a, matrix->b, matrix->c);
    printf("[%.6f, %.6f, %.6f]\n", matrix->d, matrix->e, matrix->f);
    printf("[%.6f, %.6f, %.6f]\n", 0.0, 0.0, 1.0);
}