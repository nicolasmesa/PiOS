import os
import select
import serial
import sys
import time
import tty

class UartConnection:

    def __init__(self, file_path, baud_rate):
        self.serial = serial.Serial(file_path, baud_rate)

    def send_string(self, string):
        return self.send_bytes(bytes(string, "ascii"))

    def send_bytes(self, bytes_to_send):
        return self.serial.write(bytes_to_send)

    def read(self, max_len):
        return self.serial.read(max_len)

    def read_buffer(self):
        return self.read(self.serial.in_waiting)

    def read_buffer_string(self):
        return self._decode_bytes(self.read_buffer())

    def start_interactive(self, input_file, output_file):
        try:
            tty.setcbreak(input_file.fileno())
            while True:
                rfd, _, _ = select.select([self.serial, input_file], [], [])

                if self.serial in rfd:
                    r = self.read_buffer_string()
                    output_file.write(r)
                    output_file.flush()

                if input_file in rfd:
                    r = input_file.read(1)
                    self.send_string(r)
        except KeyboardInterrupt:
            print("Got keyboard interrupt. Terminating...")
        except OSError as e:
            print("Got OSError. Terminating...")
        finally:
            os.system("stty sane")

    def _decode_bytes(self, bytes_to_decode):
        return bytes_to_decode.decode("ascii")


def main():
    import sys
    uart_connection = UartConnection(
        # Change these to match your setup
        file_path='/dev/cu.SLAB_USBtoUART',
        baud_rate=115200
    )
    time.sleep(1)
    uart_connection.start_interactive(sys.stdin, sys.stdout)

if __name__ == '__main__':
    main()