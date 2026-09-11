import queue, threading
from serial import Serial, SerialException
from serial.tools import list_ports
from PySide6.QtCore import QObject, Signal

class SerialManager(QObject):
    state_signal = Signal(bool, str)
    line_signal = Signal(str)
    raw_signal = Signal(bytes)
    error_signal = Signal(str)
    tx_signal = Signal(str)

    def __init__(self):
        super().__init__()
        self.q = queue.Queue()
        self.stop = threading.Event()
        self.ser = None
        self.buf = ''
        self.thread = threading.Thread(target=self._run, daemon=True)
        self.thread.start()

    @staticmethod
    def ports():
        return list(list_ports.comports())

    def connect(self, p, b):
        self.q.put(('connect', (p, b)))

    def disconnect(self):
        self.q.put(('disconnect', None))

    def send(self, c):
        self.q.put(('send', c.rstrip('\r\n') + '\r\n'))

    def close(self):
        self.stop.set()
        self.q.put(('shutdown', None))
        self.thread.join(2)

    def _close(self):
        was = self.ser is not None
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass
        self.ser = None
        self.buf = ''
        if was:
            self.state_signal.emit(False, '')

    def _open(self, p, b):
        self._close()
        try:
            self.ser = Serial(
                port=p,
                baudrate=b,
                bytesize=8,
                parity='N',
                stopbits=1,
                timeout=.05,
                write_timeout=1
            )
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            self.buf = ''
            self.state_signal.emit(True, f'{p} @ {b}')
        except Exception as e:
            self.ser = None
            self.error_signal.emit(f'Open failed: {e}')
            self.state_signal.emit(False, '')

    def _send(self, t):
        if not self.ser or not self.ser.is_open:
            self.error_signal.emit('Not connected.')
            return
        try:
            self.ser.write(t.encode('ascii'))
            self.ser.flush()
            self.tx_signal.emit(t.rstrip('\r\n'))
        except Exception as e:
            self.error_signal.emit(f'TX failed: {e}')

    def _rx(self):
        if not self.ser or not self.ser.is_open:
            return
        try:
            n = self.ser.in_waiting
            if not n:
                return
            data = self.ser.read(n)
            if not data:
                return

            self.raw_signal.emit(data)
            self.buf += data.decode('utf-8', 'replace')

            while '\n' in self.buf:
                line, self.buf = self.buf.split('\n', 1)
                line = line.rstrip('\r')
                if line:
                    self.line_signal.emit(line)

        except SerialException as e:
            self.error_signal.emit(f'Serial connection lost: {e}')
            self._close()
        except Exception as e:
            self.error_signal.emit(f'RX failed: {e}')

    def _run(self):
        while not self.stop.is_set():
            try:
                kind, payload = self.q.get(timeout=.02)
                if kind == 'connect':
                    self._open(*payload)
                elif kind == 'disconnect':
                    self._close()
                elif kind == 'send':
                    self._send(payload)
                elif kind == 'shutdown':
                    self._close()
                    return
            except queue.Empty:
                pass
            except Exception as e:
                self.error_signal.emit(f'Worker: {e}')

            self._rx()
