
#include <rs_config.h>
#include <rs_core.h>
#include <rs.h>

static int rs_get_options(int argc, char *const *argv);
static int      rs_show_help = 0;


int main(int argc, char * const *argv) 
{
    rs_core_info_t  *ci, *oc;

    /* init rs_errno */
    if(rs_init_strerror() != RS_OK) {
        return 1;
    }

    /* init argv */
    if(rs_get_options(argc, argv) != RS_OK) {
        rs_free_strerr();
        return 1;
    }

    if(rs_show_help == 1) {
        rs_free_strerr();
        printf( 
                "Usage: rs_[master|slave] [-?h] [-c filename]\n"
                "Options:\n"
                "   -?, -h          : this help\n"
                "   -c filename     : set configure file\n");
        return 0;
    }

    /* init core info */
    if((ci = rs_init_core_info(NULL)) == NULL) {
        rs_free_strerr();
        return 1;
    }

    rs_core_info = ci;

#if MASTER
        if(rs_init_master() != RS_OK) {
            goto free;
        }
#elif SLAVE
        if(rs_init_slave() != RS_OK) {
            goto free;
        }
#endif

    /* wait signal */
    for( ;; ) {

        if(sigwaitinfo(&(ci->sig_set), &(ci->sig_info)) == -1) {
            if(rs_errno == EINTR) {
                continue;
            }

            rs_log_error(RS_LOG_ERR, rs_errno, "sigwaitinfo() failed");
#if MASTER
            rs_free_master(NULL);
#elif SLAVE
            rs_free_slave(NULL);
#endif
            break;        
        }

        rs_sig_handle(ci->sig_info.si_signo);

        if(rs_quit) {
#if MASTER
            rs_free_master(NULL);
#elif SLAVE
            rs_free_slave(NULL);
#endif
            break;        
        } else if(rs_reload) {

            /* init core info */
            if((ci = rs_init_core_info(rs_core_info)) == NULL) {
                rs_reload = 0;
                continue;
            }

            oc = rs_core_info;

#if MASTER
            rs_init_master();
#elif SLAVE
#if 0
            rs_init_slave();
#endif
            rs_free_slave(NULL);
            break;
#endif

            if(oc != NULL) {
                rs_free_core(oc);
            }

            rs_reload = 0;

            rs_core_info = ci;

            continue;
        }
    }

free:

    if(ci->pid_path != NULL) {
        rs_delete_pidfile(ci->pid_path);
    }

    rs_free_core(ci);

    if(rs_log_fd != STDOUT_FILENO) {
        if(close(rs_log_fd) != 0) {
            rs_log_error(RS_LOG_ERR, rs_errno, "close() failed"); 
        }
    }

    rs_free_strerr();

    return 0;
}

/*
 * DESCRIPTION 
 *    Process the cmd args.
 *    -c set the configure file location
 *    -[?h] show the help information
 *
 * RETURN VALUE
 *    On success RS_OK is returned, On error RS_ERR is returned.
 */
static int rs_get_options(int argc, char *const *argv) 
{
    char   *p;
    int     i;

    for (i = 1; i < argc; i++) {

        p = argv[i];

        if (*p++ != '-') {
            rs_log_stderr(0, "invalid option: \"%s\"", argv[i]);
            return RS_ERR;
        }

        while (*p) {

            switch (*p++) {
                case '?':
                case 'h':
                    rs_show_help = 1;
                    break;

                case 'c':
                    if (*p) {
                        rs_conf_path = p;
                        goto next;
                    }

                    if (argv[++i]) {
                        rs_conf_path = argv[i];
                        goto next;
                    }

                    rs_log_stderr(0, "option \"-c\" requires config file");
                    return RS_ERR;

                default:
                    rs_log_stderr(0, "invalid option: \"%c\"", *(p - 1));
                    return RS_ERR;
            }
        }

next:

        continue;
    }

    return RS_OK;
}
