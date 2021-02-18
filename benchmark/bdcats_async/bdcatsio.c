/****** Copyright Notice ***
 *
 * PIOK - Parallel I/O Kernels - VPIC-IO, VORPAL-IO, and GCRM-IO, Copyright
 * (c) 2015, The Regents of the University of California, through Lawrence
 * Berkeley National Laboratory (subject to receipt of any required
 * approvals from the U.S. Dept. of Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Innovation & Partnerships Office
 * at  IPO@lbl.gov.
 *
 * NOTICE.  This Software was developed under funding from the U.S.
 * Department of Energy and the U.S. Government consequently retains
 * certain rights. As such, the U.S. Government has been granted for itself
 * and others acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, distribute copies to the
 * public, prepare derivative works, and perform publicly and display
 * publicly, and to permit other to do so.
 *
 ****************************/

/**
 *
 * Email questions to SByna@lbl.gov
 * Scientific Data Management Research Group
 * Lawrence Berkeley National Laboratory
 *
*/

// Description: This is a simple benchmark based on VPIC's I/O interface
//		Each process reads a specified number of particles into
//		a hdf5 output file using only HDF5 calls
// Author:	Suren Byna <SByna@lbl.gov>
//		Lawrence Berkeley National Laboratory, Berkeley, CA
// Created:	in 2011
// Modified:	01/06/2014 --> Removed all H5Part calls and using HDF5 calls
//          	02/19/2019 --> Add option to read multiple timesteps of data - Tang
//

#include <math.h>
#include <hdf5.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "h5_vol_external_async_native.h"

#define ENABLE_MPI 1

// A simple timer based on gettimeofday

#define DTYPE float

extern struct timeval start_time[3];
extern float elapse[3];
#define timer_on(id) gettimeofday (&start_time[id], NULL)
#define timer_off(id) 	\
		{	\
		     struct timeval result, now; \
		     gettimeofday (&now, NULL);  \
		     timeval_subtract(&result, &now, &start_time[id]);	\
		     elapse[id] += result.tv_sec+ (DTYPE) (result.tv_usec)/1000000.;	\
		}

#define timer_msg(id, msg) \
	printf("%f seconds elapsed in %s\n", (DTYPE)(elapse[id]), msg);  \

#define timer_reset(id) elapse[id] = 0

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */

int
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }
    if (x->tv_usec - y->tv_usec > 1000000) {
        int nsec = (y->tv_usec - x->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

struct timeval start_time[3];
float elapse[3];

// HDF5 specific declerations
herr_t ierr;

// Variables and dimensions
long numparticles = 8388608;	// 8  meg particles per process
long long total_particles, offset;

float *x, *y, *z;
float *px, *py, *pz;
int *id1, *id2;
int x_dim = 64;
int y_dim = 64;
int z_dim = 64;

// Uniform random number
inline double uniform_random_number()
{
    return (((double)rand())/((double)(RAND_MAX)));
}

void print_data(int n)
{
    int i;
    for (i = 0; i < n; i++)
        printf("%f %f %f %d %d %f %f %f\n", x[i], y[i], z[i], id1[i], id2[i], px[i], py[i], pz[i]);
}

// Create HDF5 file and read data
void read_h5_data(int rank, hid_t loc, hid_t *dset_ids, hid_t filespace, hid_t memspace, hid_t dxpl, hid_t es_id)
{
    hid_t dapl;
    dapl = H5Pcreate(H5P_DATASET_ACCESS);
#ifdef ENABLE_MPI
    H5Pset_all_coll_metadata_ops(dapl, true);
#endif

    dset_ids[0] = H5Dopen_async(loc, "x", dapl, es_id);
    dset_ids[1] = H5Dopen_async(loc, "y", dapl, es_id);
    dset_ids[2] = H5Dopen_async(loc, "z", dapl, es_id);
    dset_ids[3] = H5Dopen_async(loc, "id1", dapl, es_id);
    dset_ids[4] = H5Dopen_async(loc, "id2", dapl, es_id);
    dset_ids[5] = H5Dopen_async(loc, "px", dapl, es_id);
    dset_ids[6] = H5Dopen_async(loc, "py", dapl, es_id);
    dset_ids[7] = H5Dopen_async(loc, "pz", dapl, es_id);

    ierr = H5Dread_async(dset_ids[0], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, x, es_id);

    ierr = H5Dread_async(dset_ids[1], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, y, es_id);

    ierr = H5Dread_async(dset_ids[2], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, z, es_id);

    ierr = H5Dread_async(dset_ids[3], H5T_NATIVE_INT, memspace, filespace, dxpl, id1, es_id);

    ierr = H5Dread_async(dset_ids[4], H5T_NATIVE_INT, memspace, filespace, dxpl, id2, es_id);

    ierr = H5Dread_async(dset_ids[5], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, px, es_id);

    ierr = H5Dread_async(dset_ids[6], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, py, es_id);

    ierr = H5Dread_async(dset_ids[7], H5T_NATIVE_FLOAT, memspace, filespace, dxpl, pz, es_id);

    if (rank == 0) printf ("  Read 8 variable completed\n");

    H5Pclose(dapl);
    //print_data(3);
}

void print_usage(char *name)
{
    printf("Usage: %s /path/to/file #timestep sleep_sec [# mega particles]\n", name);
}

int main (int argc, char* argv[])
{
    int my_rank, num_procs, nts, i, j, sleep_time;
    hid_t file_id, *grp_ids, **dset_ids, async_dxpl, es_id;
    hid_t filespace, memspace;
    hid_t async_fapl, gapl;

#ifdef ENABLE_MPI
    /* MPI_Init(&argc,&argv); */
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if(argc < 3) {
        printf("Usage: ./%s /path/to/file #timestep [# mega particles]\n", argv[0]);
        return 0;
    }
    MPI_Comm_rank (MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size (MPI_COMM_WORLD, &num_procs);
    MPI_Info info  = MPI_INFO_NULL;
#endif

    char *file_name = argv[1];
    char grp_name[128];

    if (argc < 4) {
        print_usage(argv[0]);
        return 0;
    }

    nts = atoi(argv[2]);
    if (nts <= 0) {
        printf("Usage: ./%s /path/to/file #timestep [# mega particles]\n", argv[0]);
        return 0;
    }

    sleep_time = atoi(argv[3]);
    if (sleep_time < 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (argc == 5) {
        numparticles = (atoi (argv[4]))*1024*1024;
    }
    else {
        numparticles = 8*1024*1024;
    }


    if (my_rank == 0) {
        printf ("Number of paritcles: %ld \n", numparticles);
    }

    x=(float*)malloc(numparticles*sizeof(double));
    y=(float*)malloc(numparticles*sizeof(double));
    z=(float*)malloc(numparticles*sizeof(double));

    px=(float*)malloc(numparticles*sizeof(double));
    py=(float*)malloc(numparticles*sizeof(double));
    pz=(float*)malloc(numparticles*sizeof(double));

    id1=(int*)malloc(numparticles*sizeof(int));
    id2=(int*)malloc(numparticles*sizeof(int));

#ifdef ENABLE_MPI
    MPI_Barrier (MPI_COMM_WORLD);
#endif
    timer_on (0);

    total_particles = numparticles;
    offset = 0;
#ifdef ENABLE_MPI
    MPI_Allreduce(&numparticles, &total_particles, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    MPI_Scan(&numparticles, &offset, 1, MPI_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);
    offset -= numparticles;
#endif


    /* Set up FAPL */
    async_fapl = H5Pcreate (H5P_FILE_ACCESS);
    async_dxpl = H5Pcreate (H5P_DATASET_XFER);
    /* H5Pset_vol_async(async_fapl); */

#ifdef ENABLE_MPI
    H5Pset_fapl_mpio(async_fapl, MPI_COMM_WORLD, info);
    H5Pset_all_coll_metadata_ops(async_fapl, true);
#endif

    if((gapl = H5Pcreate(H5P_GROUP_ACCESS)) < 0)
        goto error;
#ifdef ENABLE_MPI
    if(H5Pset_all_coll_metadata_ops(gapl, true) < 0)
        goto error;
#endif

    es_id = H5EScreate(); 

    /* Open file */
    file_id = H5Fopen_async(file_name, H5F_ACC_RDONLY, async_fapl, es_id);
    if(file_id < 0) {
        printf("Error with opening file [%s]!\n", file_name);
        goto done;
    }

    if (my_rank == 0)
        printf ("Opened HDF5 file ... [%s]\n", file_name);

    filespace = H5Screate_simple(1, (hsize_t *) &total_particles, NULL);
    memspace =  H5Screate_simple(1, (hsize_t *) &numparticles, NULL);

    //printf("total_particles: %lld\n", total_particles);
    //printf("my particles   : %ld\n", numparticles);

    //H5Pset_async_dxpl_mpio(async_faplfapl, H5FD_MPIO_COLLECTIVE);
    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, (hsize_t *) &offset, NULL, (hsize_t *) &numparticles, NULL);
    
    dset_ids = (hid_t**)calloc(nts, sizeof(hid_t*));
    grp_ids  = (hid_t*)calloc(nts, sizeof(hid_t));
    for (i = 0; i < nts; i++) 
        dset_ids[i] = (hid_t*)calloc(8, sizeof(hid_t));

    /* MPI_Barrier (MPI_COMM_WORLD); */
    timer_on (1);

    i = 0;
    sprintf(grp_name, "Timestep_%d", i);
    grp_ids[i] = H5Gopen_async(file_id, grp_name, H5P_DEFAULT, es_id);
    if (my_rank == 0)
        printf ("Reading %s ... \n", grp_name);
    read_h5_data(my_rank, grp_ids[i], dset_ids[i], filespace, memspace, async_dxpl, es_id);

    for (i = 1; i < nts; i++) {
        timer_reset(2);
        timer_on (2);
        sprintf(grp_name, "Timestep_%d", i);
        grp_ids[i] = H5Gopen_async(file_id, grp_name, gapl, es_id);

        if (my_rank == 0)
            printf ("Prefetch %s ... \n", grp_name);

        read_h5_data(my_rank, grp_ids[i], dset_ids[i], filespace, memspace, async_dxpl, es_id);

        /* H5VL_async_start(); */

        if (i != 0) {
            if (my_rank == 0) printf ("  sleep for %ds\n", sleep_time);
            sleep(sleep_time);
        }

        if (i == 1) {
            for (j = 0; j < 8; j++) 
                H5Dclose_async(dset_ids[0][j], es_id);
            H5Gclose_async(grp_ids[0], es_id);
        }

        for (j = 0; j < 8; j++) 
            H5Dclose_async(dset_ids[i][j], es_id);
        H5Gclose_async(grp_ids[i], es_id);

        /* MPI_Barrier (MPI_COMM_WORLD); */
        timer_off(2);
        if (my_rank == 0) 
            timer_msg (2, "prefetch read and wait 1 timestep");

    }

#ifdef ENABLE_MPI
    MPI_Barrier (MPI_COMM_WORLD);
#endif
    timer_off (1);


    H5Sclose(memspace);
    H5Sclose(filespace);
    H5Pclose(async_fapl);
    H5Pclose(async_dxpl);
    H5Pclose(gapl);

    H5Fclose_async(file_id, es_id);

    size_t num_in_progress;
    hbool_t op_failed;
    H5ESwait(es_id, H5ES_WAIT_FOREVER, &num_in_progress, &op_failed);
    H5ESclose(es_id);

#ifdef ENABLE_MPI
    MPI_Barrier (MPI_COMM_WORLD);
#endif
    timer_off (0);
    if (my_rank == 0)
    {
        printf ("\nTiming results\n");
        printf("Total sleep time %ds\n", sleep_time*(nts-1));
        timer_msg (1, "just reading data");
        timer_msg (0, "opening, reading, closing file");
        printf ("\n");
    }

    free(x);
    free(y);
    free(z);
    free(px);
    free(py);
    free(pz);
    free(id1);
    free(id2);
    for (i = 0; i < nts; i++) 
        free(dset_ids[i]);
    free(dset_ids);
    free(grp_ids);

    goto done;

error:
    H5E_BEGIN_TRY {
        H5Fclose(file_id);
        H5Pclose(async_fapl);
    } H5E_END_TRY;

done:
    MPI_Finalize();

    return 0;
}
