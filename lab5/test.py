from mpi4py import MPI
from numpy import empty, array, int32, float64

comm = MPI.COMM_WORLD
numprocs = comm.Get_size()
rank = comm.Get_rank()


def parallel_tridiagonal_matrix_algorithm(a_part, b_part, c_part, d_part):
    N_part = len(d_part)

    for n in range(1, N_part):
        coef = a_part[n] / b_part[n - 1]
        a_part[n] = -coef * a_part[n - 1]
        b_part[n] = b_part[n] - coef * c_part[n - 1]
        d_part[n] = d_part[n] - coef * d_part[n - 1]

    for n in range(N_part - 3, -1, -1):
        coef = c_part[n] / b_part[n + 1]
        c_part[n] = -coef * c_part[n + 1]
        a_part[n] = a_part[n] - coef * a_part[n + 1]
        d_part[n] = d_part[n] - coef * d_part[n + 1]

    if rank > 0:
        temp_array_send = array([a_part[0], b_part[0],
                                 c_part[0], d_part[0]], dtype=float64)
    if rank < numprocs - 1:
        temp_array_recv = empty(4, dtype=float64)

    if rank == 0:
        comm.Recv([temp_array_recv, 4, MPI.DOUBLE], source=1, tag=0, status=None)

    if rank in range(1, numprocs - 1):
        comm.Sendrecv(sendbuf=[temp_array_send, 4, MPI.DOUBLE], dest=rank - 1, sendtag=0,
                      recvbuf=[temp_array_recv, 4, MPI.DOUBLE], source=rank + 1, recvtag=MPI.ANY_TAG, status=None)

    if rank == numprocs - 1:
        comm.Send([temp_array_send, 4, MPI.DOUBLE], dest=numprocs - 2, tag=0)

    if rank < numprocs - 1:
        coef = c_part[N_part - 1] / temp_array_recv[1]
        b_part[N_part - 1] = b_part[N_part - 1] - coef * temp_array_recv[0]
        c_part[N_part - 1] = - coef * temp_array_recv[2]
        d_part[N_part - 1] = d_part[N_part - 1] - coef * temp_array_recv[3]

    temp_array_send = array([a_part[N_part - 1], b_part[N_part - 1],
                             c_part[N_part - 1], d_part[N_part - 1]], dtype=float64)

    if rank == 0:
        A_extended = empty((numprocs, 4), dtype=float64)
    else:
        A_extended = None

    comm.Gather([temp_array_send, 4, MPI.DOUBLE], [A_extended, 4, MPI.DOUBLE], root=0)

    if rank == 0:
        x_temp = consecutive_tridiagonal_matrix_algorithm(A_extended[:, 0], A_extended[:, 1], A_extended[:, 2],
                                                          A_extended[:, 3])
    else:
        x_temp = None

    if rank == 0:
        rcounts_temp = empty(numprocs, dtype=int32)
        displs_temp = empty(numprocs, dtype=int32)
        rcounts_temp[0] = 1
        displs_temp[0] = 0
        for k in range(1, numprocs):
            rcounts_temp[k] = 2
            displs_temp[k] = k - 1
    else:
        rcounts_temp = None;
        displs_temp = None

    if rank == 0:
        x_part_last = empty(1, dtype=float64)
        comm.Scatterv([x_temp, rcounts_temp, displs_temp, MPI.DOUBLE], [x_part_last, 1, MPI.DOUBLE], root=0)
    else:
        x_part_last = empty(2, dtype=float64)
        comm.Scatterv([x_temp, rcounts_temp, displs_temp, MPI.DOUBLE], [x_part_last, 2, MPI.DOUBLE], root=0)

    x_part = empty(N_part, dtype=float64)

    if rank == 0:
        for n in range(N_part - 1):
            x_part[n] = (d_part[n] - c_part[n] * x_part_last[0]) / b_part[n]
        x_part[N_part - 1] = x_part_last[0]
    else:
        for n in range(N_part - 1):
            x_part[n] = (d_part[n] - a_part[n] * x_part_last[0] - c_part[n] * x_part_last[1]) / b_part[n]
        x_part[N_part - 1] = x_part_last[1]

    return x_part


def main():
    a_part, b_part, c_part, d_part = [], [], [], []
    parallel_tridiagonal_matrix_algorithm(a_part, b_part, c_part, d_part)


if __name__ == "__main__":
    main()
