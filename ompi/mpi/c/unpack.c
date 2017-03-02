/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2016 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015-2017 Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/datatype/ompi_datatype.h"
#include "opal/datatype/opal_convertor.h"
#include "ompi/memchecker.h"

#if OMPI_BUILD_MPI_PROFILING
#if OPAL_HAVE_WEAK_SYMBOLS
#pragma weak MPI_Unpack = PMPI_Unpack
#endif
#define MPI_Unpack PMPI_Unpack
#endif

static const char FUNC_NAME[] = "MPI_Unpack";


int MPI_Unpack(const void *inbuf, int insize, int *position,
               void *outbuf, int outcount, MPI_Datatype datatype,
               MPI_Comm comm)
{
    int rc = MPI_SUCCESS;
    opal_convertor_t local_convertor;
    struct iovec outvec;
    unsigned int iov_count;
    size_t size;

    MEMCHECKER(
        memchecker_datatype(datatype);
        memchecker_call(&opal_memchecker_base_isdefined, outbuf, outcount, datatype);
        memchecker_comm(comm);
    );

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_comm_invalid(comm)) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM,
                                          FUNC_NAME);
        }

        if ((NULL == inbuf) || (NULL == position)) {  /* outbuf can be MPI_BOTTOM */
            return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_ARG, FUNC_NAME);
        }

        if (outcount < 0) {
            return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_COUNT, FUNC_NAME);
        }

        OMPI_CHECK_DATATYPE_FOR_RECV(rc, datatype, outcount);
        OMPI_ERRHANDLER_CHECK(rc, comm, rc, FUNC_NAME);
        OMPI_CHECK_USER_BUFFER(rc, outbuf, datatype, outcount);
        OMPI_ERRHANDLER_CHECK(rc, comm, rc, FUNC_NAME);
    }


    if( insize > 0 ) {
        int ret;
        OBJ_CONSTRUCT( &local_convertor, opal_convertor_t );
        /* the resulting convertor will be set the the position ZERO */
        opal_convertor_copy_and_prepare_for_recv( ompi_mpi_local_convertor, &(datatype->super),
                                                  outcount, outbuf, 0, &local_convertor );

        /* Check for truncation */
        opal_convertor_get_packed_size( &local_convertor, &size );
        if( (*position + size) > (unsigned int)insize ) {
            OBJ_DESTRUCT( &local_convertor );
            return OMPI_ERRHANDLER_INVOKE(comm, MPI_ERR_TRUNCATE, FUNC_NAME);
        }

        /* Prepare the iovec with all informations */
        outvec.iov_base = (char*) inbuf + (*position);
        outvec.iov_len = size;

        /* Do the actual unpacking */
        iov_count = 1;
        ret = opal_convertor_unpack( &local_convertor, &outvec, &iov_count, &size );
        *position += size;
        OBJ_DESTRUCT( &local_convertor );
        /* All done.  Note that the convertor returns 1 upon success, not
           OPAL_SUCCESS. */
        if (1 != ret) {
            rc = OMPI_ERROR;
        }
    }

    OMPI_ERRHANDLER_RETURN(rc, comm, MPI_ERR_UNKNOWN, FUNC_NAME);
}
