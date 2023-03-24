//
// Created by tanmay on 8/10/22.
//

#include <stdio.h>
#include <stdlib.h>

double RHO = 12;
double SIGMA = 10;
double BETA = 8/3;

#define type double
#define type_precision_format "%0.15lf"

void solve_lorenz(type x_0, type y_0, type z_0) {
  type x_1 = 0, y_1 = 0, z_1 = 0;
  int n = 100;

  char command_filename[] = "lorenz_ode_commands.txt";
  FILE *command_unit;
  char data_filename[] = "lorenz_ode_data.txt";
  FILE *data_unit;

  data_unit = fopen ( data_filename, "wt" );

  for(int i = 0; i < n; i++) {
    //    printf("x_0 = %0.15f\n", x_0);
    //    printf("y_0 = %0.15f\n", y_0);
    //    printf("z_0 = %0.15lf\n\n", z_0);

    fprintf ( data_unit, "  %d  %14.6g  %14.6g  %14.6g\n",
            2*i, x_0, y_0, z_0 );

    x_1 = x_0 + SIGMA*(y_0-x_0)*0.005;
    y_1 = y_0 + (RHO*x_0 - y_0 - x_0*z_0)*0.005;
    z_1 = z_0 + (x_0*y_0 - BETA*z_0)*0.005;

    //    printf("x_1 = %0.15f\n", x_1);
    //    printf("y_1 = %0.15f\n", y_1);
    //    printf("z_1 = %0.15f\n\n", z_1);

    fprintf ( data_unit, "  %d  %14.6g  %14.6g  %14.6g\n",
            2*i+1, x_1, y_1, z_1 );

    x_0 = x_1 + SIGMA*(y_1-x_1)*0.005;
    y_0 = y_1 + (RHO*x_1 - y_1 - x_1*z_1)*0.005;
    z_0 = z_1 + (x_1*y_1 - BETA*z_1)*0.005;
  }

  printf("x_0 = "type_precision_format"\n", x_0);
  printf("y_0 = "type_precision_format"\n", y_0);
  printf("z_0 = "type_precision_format"\n\n", z_0);

  fprintf ( data_unit, "  %d  %14.6g  %14.6g  %14.6g\n",
          2*n, x_0, y_0, z_0 );
}

int main() {
  type x_0 = 1.2;
  type y_0 = 1.3;
  type z_0 = 1.4;
//  printf("Enter value of z: ");
//  scanf("%lf", &z_0);

  solve_lorenz(x_0, y_0, z_0);

//  printf("z_1 = "type_precision_format"\n\n", z_1);
}