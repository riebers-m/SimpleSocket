import socket
import threading
import time


def ping(num: int):
    client_socket: socket.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect(("localhost", 12345))
    start = time.time()
    while (time.time() - start) < 8:
        client_socket.sendall(f"ping".encode())
        try:
            recieved: bytes = client_socket.recv(1024)
            if recieved == 0:
                print(f"t{num}: connection was closed")
                return
            print(f"t{num} received: {recieved.decode()}")
        except Exception as e:
            print(f"exception caught: {e}")
        time.sleep(0.1)


def start_thread(threads_to_start: int) -> list[threading.Thread]:
    thread_list = []
    for index in range(threads_to_start):
        thrd = threading.Thread(target=ping, args=(index,))
        thrd.start()
        thread_list.append(thrd)
    return thread_list


if __name__ == "__main__":
    threads = start_thread(10)

    time.sleep(9)
    for thread in threads:
        thread.join()
