//-< W32SOCK.H >-----------------------------------------------------*--------*
// GigaBASE                  Version 1.0         (c) 1999  GARRET    *     ?  *
// (Post Relational Database Management System)                      *   /\|  *
//                                                                   *  /  \  *
//                          Created:      8-May-97    K.A. Knizhnik  * / [] \ *
//                          Last update:  8-May-97    K.A. Knizhnik  * GARRET *
//-------------------------------------------------------------------*--------*
// Windows sockets
//-------------------------------------------------------------------*--------*

#ifndef __W32SOCK_H__
#define __W32SOCK_H__

#include "sockio.h"

BEGIN_GIGABASE_NAMESPACE

class win_socket : public socket_t {
  protected:
    SOCKET        s;

    enum error_codes {
        ok = 0,
        not_opened = -1,
        bad_address = -2,
        connection_failed = -3,
        broken_pipe = -4,
        invalid_access_mode = -5
    };

  public:
    bool      open(int listen_queue_size);
    bool      connect(int max_attempts, time_t timeout);

    int       read(void* buf, size_t min_size, size_t max_size,time_t timeout);
    bool      write(void const* buf, size_t size);

    bool      is_ok();
    bool      close();
    char*     get_peer_name();
    bool      shutdown();
    void      get_error_text(char_t* buf, size_t buf_size);

    socket_t* accept();
    bool      cancel_accept();

    win_socket(const char* address);
    win_socket(SOCKET new_sock);

    ~win_socket();
};

#define SOCKET_BUF_SIZE (8*1024)
#define ACCEPT_TIMEOUT  (30*1000)

class local_win_socket : public socket_t {
  protected:
    enum error_codes {
        ok = 0,
        not_opened = -1,
        broken_pipe = -2,
        timeout_expired = -3
    };
    enum socket_signals {
        RD,  // receive data
        RTR, // ready to receive
        TD,  // transfer data
        RTT  // ready to transfer
    };
    //------------------------------------------------------
    // Mapping between signals at opposite ends of socket:
    // TD  ---> RD
    // RTR ---> RTT
    //------------------------------------------------------

    struct socket_buf {
        volatile int RcvWaitFlag;
        volatile int SndWaitFlag;
        volatile int DataEnd;
        volatile int DataBeg;
        char Data[SOCKET_BUF_SIZE - 4*sizeof(int)];
    };
    struct accept_data {
        HANDLE Signal[4];
        HANDLE BufHnd;
    };
    struct connect_data {
        HANDLE Mutex;
        int    Pid;
    };
    socket_buf* RcvBuf;
    socket_buf* SndBuf;
    HANDLE      Signal[4];
    HANDLE      Mutex;
    HANDLE      BufHnd;
    int         Error;
    char*     Name;

  public:
    bool      open(int listen_queue_size);
    bool      connect(int max_attempts, time_t timeout);

    int       read(void* buf, size_t min_size, size_t max_size,time_t timeout);
    bool      write(void const* buf, size_t size);

    char*   get_peer_name();
    bool      is_ok();
    bool      close();
    bool      shutdown();
    void      get_error_text(char_t* buf, size_t buf_size);

    socket_t* accept();
    bool      cancel_accept();

    local_win_socket(const char* address);
    local_win_socket();

    ~local_win_socket();
};

END_GIGABASE_NAMESPACE

#endif
