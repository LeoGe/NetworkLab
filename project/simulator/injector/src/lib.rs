extern crate libc;
extern crate priority_queue;
extern crate errno;

#[macro_use]
extern crate lazy_static;

#[macro_use]
extern crate redhook;

mod state;

use std::ptr;
use std::net::{Ipv4Addr, SocketAddrV4};
use libc::{c_int, c_void, size_t, ssize_t, sockaddr, socklen_t,AF_INET, sockaddr_in, epoll_event, EPOLL_CTL_ADD, EPOLLIN, EPOLLOUT, EPOLLRDHUP, EPOLLPRI, EPOLLERR, EPOLLHUP, EPOLLET, EPOLLONESHOT, EPOLLWAKEUP, EPOLLEXCLUSIVE, c_uint, mode_t, EAGAIN, EWOULDBLOCK, iovec};
use errno::{set_errno, Errno};

use std::sync::{Condvar, Mutex, atomic::AtomicBool, atomic::Ordering};
use std::slice;
use state::{EpollWait, State};
use std::time::Duration;

lazy_static! {
    static ref STATE: Mutex<State> = Mutex::new(State::new());
    static ref SYNC: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
    static ref WAIT_FOR_FD: (Mutex<bool>, Condvar) = (Mutex::new(false), Condvar::new());
    static ref PENDING: AtomicBool= AtomicBool::new(false);
}

hook! {
    unsafe fn get_state() -> State => fake_get_state {
        STATE.lock().unwrap().clone()
    }
}

hook! {
    unsafe fn sendto(fd: c_int, buf: *const c_void, len: size_t, flags: c_int, dest_addr: *const sockaddr, addrlen: socklen_t) {
    }
}

hook! {
    unsafe fn recvfrom(fd: c_int, buf: *mut c_void, len: size_t, flags: c_int, src_addr: *mut sockaddr, addrlen: *mut socklent_t) {
    }
}

hook! {
    unsafe fn bind(ssocket: c_int, address: *const sockaddr, _address_len: socklen_t) -> c_int => fake_bind {
        if (*address).sa_family == AF_INET as u16 {
            let addr = address as *const sockaddr_in;

            // convert host and port to SocketAddrV4
            let port = (*addr).sin_port.to_be();
            let addr = Ipv4Addr::from(((*addr).sin_addr.s_addr as u32).to_be());

            STATE.lock().unwrap().add_node(ssocket, SocketAddrV4::new(addr, port));
        } else {
            panic!("We're only supporting the IPv4 address space");
        }

        0
    }
}

fn events_to_string(events: c_uint) -> String {
    [
        ("EPOLLIN", EPOLLIN), 
        ("EPOLLOUT", EPOLLOUT),
        ("EPOLLRDHUP", EPOLLRDHUP),
        ("EPOLLPRI", EPOLLPRI),
        ("EPOLLERR", EPOLLERR),
        ("EPOLLHUP", EPOLLHUP),
        ("EPOLLET", EPOLLET),
        ("EPOLLONESHOT", EPOLLONESHOT),
        ("EPOLlWAKEUP", EPOLLWAKEUP),
        ("EPOLLEXCLUSIVE", EPOLLEXCLUSIVE)
    ].iter().filter_map(|(a,b)| {
        if (events & (*b) as u32) != 0 {
            return Some((*a).into());
        } else {
            return None;
        }
    }).collect::<Vec<String>>().join(", ")
}

hook! {
    unsafe fn epoll_ctl(epfd: c_int, op: c_int, fd: c_int, event: *mut epoll_event) -> c_int => fake_epoll_ctl {
        if op == EPOLL_CTL_ADD {
            let events = (*event).events;

            println!("Hook: register {} with id {}", fd, (*event).u64);

            STATE.lock().unwrap().add_epoll_fd(fd, events, (*event).u64);

            // in case we're waiting for an FD to be ready
            WAIT_FOR_FD.1.notify_one();

        }

        0
    }
}

hook! {
    unsafe fn epoll_wait(_epfd: c_int, events: *mut epoll_event, _maxevents: c_int, _timeout: c_int) -> c_int => fake_epoll_wait {
        let mut started = SYNC.0.lock().unwrap();
        let mut started2 = WAIT_FOR_FD.0.lock().unwrap();
        'outer: loop {
        // push all fd events out
        'inner: loop {
            match STATE.lock().unwrap().next_epoll_notify() {
                EpollWait::Ready((epoll_id, epoll_events)) => {
                    let answ = epoll_event { 
                        events: epoll_events as u32, 
                        u64: epoll_id
                    };

                    ptr::write(events, answ);

                    return 1;
                },
                EpollWait::NotReady(a) => {
                    println!("epoll wait for {}", a);
                },
                EpollWait::Empty => {
                    break 'inner;
                }
            }

            started2 = WAIT_FOR_FD.1.wait(started2).unwrap();
        }

        if PENDING.load(Ordering::Relaxed) {
            println!(" # pending start");
            started = SYNC.1.wait(started).unwrap();
            println!(" # pending end");
        }

        match STATE.lock().unwrap().next_epoll_id() {
            EpollWait::Ready((epoll_id, epoll_events)) => {
                PENDING.store(true, Ordering::Relaxed);

                let answ = epoll_event { 
                    events: epoll_events as u32, 
                    u64: epoll_id
                };

                ptr::write(events, answ);

                return 1;
            },
            EpollWait::NotReady(a) => {
                println!("event wait for {}", a);
                //continue;
            },
            EpollWait::Empty => {
                break 'outer;
            }
        }

        }

        let ended = STATE.lock().unwrap().has_ended();
        if ended {
            STATE.lock().unwrap().print_log();
            //started2 = WAIT_FOR_FD.1.wait(started2).unwrap();
            started = SYNC.1.wait(started).unwrap();
        }
        return 0;
    }
}

hook! {
    unsafe fn creat(path: *const u8, mode: mode_t) -> c_int => fake_creat {
        let fd = real!(creat)(path, mode);

        println!("Hook: creat - ask for logs");

        //let buf = STATE.lock().unwrap().get_logs();
        //real!(write)(fd, buf.as_ptr() as *const c_void, buf.len());

        fd
    }
}
