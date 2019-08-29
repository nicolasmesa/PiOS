import argparse
import os
import select
import serial
import sys
import time
import tty

# TODO: Make ti work with contexts (with UartConnection() as u)


class UartConnection:

    def __init__(self, file_path, baud_rate):
        self.serial = serial.Serial(file_path, baud_rate)

    def send_line(self, line):
        if not line.endswith("\n"):
            # Intentionally not adding the \r for now
            line += "\n"
        return self.send_string(line)

    def send_string(self, string):
        return self.send_bytes(bytes(string, "ascii"))

    def send_bytes(self, bytes_to_send):
        return self.serial.write(bytes_to_send)

    def send_int(self, number):
        if number > 2 ** 32 - 1:
            raise 'Number can only be 4 bytes long'
        number_in_bytes = number.to_bytes(4, byteorder='big')
        return self.send_bytes(number_in_bytes)

    def read(self, max_len):
        return self.serial.read(max_len)

    def read_buffer(self):
        return self.read(self.serial.in_waiting)

    def read_buffer_string(self):
        return self._decode_bytes(self.read_buffer())

    def read_line(self):
        return self._decode_bytes(self.serial.readline())

    def read_int(self):
        bytes_to_read = 4
        number_bytes = self.read(bytes_to_read)
        return int.from_bytes(number_bytes, byteorder='big')

    def start_interactive(self, input_file, output_file):
        try:
             # Make the tty cbreak
             # https://www.oreilly.com/library/view/python-standard-library/0596000960/ch12s08.html
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
            return
        except OSError as e:
            print("Got OSError. Terminating...")
            return
        finally:
            os.system("stty sane")

    def close(self):
        self.serial.close()

    def _decode_bytes(self, bytes_to_decode):
        return bytes_to_decode.decode("ascii")


def compute_kernel_checksum(kernel_bytes):
    num = 0
    for b in kernel_bytes:
        num = (num + b) % (2 ** 32)
    return num

def send_kernel_debug(uart_connection, kernel):
    for i, b in enumerate(kernel):
        print(i, 'Sending byte', b)
        uart_connection.send_bytes(bytes([b]))
        read_byte = uart_connection.read(1)[0]
        print(i, 'Received byte', read_byte)

        if b != read_byte:
            print(i, 'Sent', b, 'but got', read_byte)
            return False

    return True

def send_kernel(path, uart_connection, debug=False):
    with open(path, mode='rb') as f:
        uart_connection.send_line("kernel")
        time.sleep(1)

        kernel = f.read()
        size = len(kernel)
        checksum = compute_kernel_checksum(kernel)

        print("Sending kernel with size", size, "and checksum", checksum)
        uart_connection.send_int(size)
        time.sleep(1)
        size_confirmation = uart_connection.read_int()

        if size_confirmation != size:
            print("Expected size to be", size, "but got", size_confirmation)
            return False

        print("Kernel size confirmed. Sending kernel")
        if debug:
            print("Starting debug workflow")
            uart_connection.send_int(1)
            send_kernel_debug(uart_connection, kernel)
        else:
            uart_connection.send_int(0)
            uart_connection.send_bytes(kernel)

        time.sleep(1)

        print("Validating checksum...")
        checksum_confirmation = uart_connection.read_int()
        if checksum_confirmation != checksum:
            print("Expected checksum to be", checksum,
                  "but was", checksum_confirmation)
            return False

        line = uart_connection.read_line()
        print("Received: ", line)
        if not line.startswith("Done"):
            print("Didn't get confirmation for the kernel. Got", line)
            return False

        return True


def main(argv):
    ap = argparse.ArgumentParser(
        description="""
            Utility program to send the kernel to the RPI over UART and to start an interactive session over uart.
            Sample usage:
            The following command will setup a serial connection and will send the kernel over it. Then, it
            will start an interactive session over the serial connection.
                python boot_send.py -d /dev/cu.SLAB_USBtoUART -b 115200 -k ../kernel8.img -i
            """
    )
    ap.add_argument('-d', '--device', help='path to RPI UART device', required=True,
                    default='/dev/cu.SLAB_USBtoUART')
    ap.add_argument('-b', '--baud-rate',
                    help='baud rate to use for the UART communication', required=True, type=int,
                    default=115200)
    ap.add_argument('-k', '--kernel', help='file path to the kernel', required=False,
                    type=str)
    ap.add_argument('-i', '--interactive', help='start interactive session',
                    action='store_const', const=True, default=False)
    ap.add_argument('-dd', '--debug', const=True, default=False, action='store_const')

    args = ap.parse_args(argv[1:])

    if not args.kernel and not args.interactive:
        print("At least one of '--kernel' or '--interactive' are required")
        sys.exit(1)

    uart_connection = UartConnection(args.device, args.baud_rate)
    time.sleep(1)

    if args.kernel:
        success = send_kernel(args.kernel, uart_connection, args.debug)
        if not success:
            sys.exit(1)
        time.sleep(1)

    if args.interactive:
        print("Making it interactive. Press ctrl-c to exit. You may need to press enter...")
        uart_connection.start_interactive(sys.stdin, sys.stdout)

    uart_connection.close()


if __name__ == '__main__':
    main(sys.argv)
