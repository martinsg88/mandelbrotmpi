#include <stdio.h>
#include <sys/time.h>
#include <vector>
#include "mpi.h"

using namespace std;

#define width  8000	
#define height 8000

#define DATA  1
#define RESULT  2
#define TERMINATE_TAG  3

struct complex{
	float real;
	float imag;
};

int cal_pixel(complex c){
	
	int idle, max_iter;
	complex z;

	float temp, length;
	max_iter = 255;
	z.real = 0;
	z.imag = 0;
	idle = 0;

	do{
		temp = z.real * z.real - z.imag * z.imag + c.real;
		z.imag = 2 * z.real * z.imag + c.imag;
		z.real = temp;
		length = z.real * z.real + z.imag * z.imag;
		idle++;
	}while((length<4.0) && (idle<max_iter));

	return idle;
}

void print_image(vector<vector<int> > &image){

	FILE* fp;
	fp = fopen("parallel.PPM", "w");
	fprintf(fp,"P2\n");
	fprintf(fp,"%d %d\n", width, height); 	
	fprintf(fp,"255\n");
		
        for(int i=0; i<width; i++){
                for(int j=0; j<height; j++){
                	fprintf(fp,"%d ", image[i][j]); 	
		}	
		fprintf(fp,"\n"); 	     
	}
}

void master(vector<vector<int> > &image, int slaves){
	
	int idle = 0;
	int row = 0;
	int arrcal[width]; 
	int sends = 1;
	MPI_Status status;
		
	for(int i = 1; i < slaves; i++){
		MPI_Send(&row, 1, MPI_INT, i, DATA, MPI_COMM_WORLD);			
		idle++;
		row++;
	}
	do{
		MPI_Recv(arrcal, width, MPI_INT, idle, RESULT, MPI_COMM_WORLD, &status);
		MPI_Recv(&row, 1, MPI_INT, idle, RESULT, MPI_COMM_WORLD, &status);	
		idle--;
		if(row<width){
			for(int i=0; i<width; i++){
				image[row][i] = arrcal[i]; 
			}
			row++;
			MPI_Send(&row, 1, MPI_INT, status.MPI_SOURCE, DATA, MPI_COMM_WORLD);	
			idle++;
		}
		else{
			for(int i=1; i < slaves; i++){
				MPI_Send(0, 0, MPI_INT, i, TERMINATE_TAG, MPI_COMM_WORLD);
			}
		break;
		}
	}while(idle > 0);

print_image(image);
}

void slave(vector<vector<int> > &image){
	float real_min=-2, imag_min = -2;
	float real_max=2, imag_max = 2;
	float scale_real = (real_max - real_min)/width;
	float scale_imag = (imag_max - imag_min)/height;		  
	complex c;	
	int arrcal[width];
	int row;
	int k = 0;
	
	MPI_Status status;
	
	MPI_Recv(&row, width, MPI_INT, 0, DATA, MPI_COMM_WORLD, &status);	

	while(status.MPI_TAG==DATA){
		int i = row;
		c.imag = imag_min + ((float) i*scale_imag);
		for(int j=0; j<width; j++){	                                	
			c.real = real_min + ((float) j*scale_real);	
			arrcal[j] = cal_pixel(c);    
		}	
		MPI_Send(arrcal, width, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
		MPI_Send(&row, 1, MPI_INT, 0, RESULT, MPI_COMM_WORLD);
		MPI_Recv(&row, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);							
		
		if(status.MPI_TAG==TERMINATE_TAG){
			return;
		}
	}
}

int main( int argc, char **argv ){
	
	int rank, slaves;	
			
	struct timeval tim1, tim2;
	double t1, t2;
	MPI_Init(&argc, &argv );
	MPI_Comm_size(MPI_COMM_WORLD, &slaves);	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	vector< vector<int> > array;
	
	array.resize(height);
	for(int i = 0; i < height; i++)
		array[i].resize(width);

	if(rank==0){	
		clock_t start = clock();	
		master(array, slaves);		
		clock_t end = clock();	
		double time_elasped_in_seconds = (end - start)/(double)CLOCKS_PER_SEC;
		printf("Time Taken Total: %f\n", time_elasped_in_seconds);
	}
	else{
		slave(array);
	}

MPI_Finalize();
return 0;
}
