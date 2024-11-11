#!/usr/bin/env python3
import sys
from PySide6 import QtCore, QtGui, QtWidgets, QtSerialPort
from PySide6.QtCore import QTimer
from PIL import Image, ImageQt
from enum import Enum


DISPLAY_WIDTH = 128
DISPLAY_HEIGHT = 64
PORT = "/dev/cu.usbmodem01"
SCALE = 4


class Key(Enum):
    Up = 0
    Down = 1
    Right = 2
    Left = 3
    Ok = 4
    Back = 5


class Widget(QtWidgets.QWidget):
    def __init__(self, parent=None):
        self.buffer = b""
        self.last_frame_count = 0

        super(Widget, self).__init__(parent)
        self.image = QtWidgets.QLabel()
        self.image.setScaledContents(True)
        self.image.setPixmap(
            ImageQt.toqpixmap(
                Image.new(
                    "RGB",
                    (DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE),
                    (255, 255, 255),
                )
            )
        )

        self.bars = Image.open("bars.png")
        self.bars = self.bars.resize(
            (DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE), resample=Image.NEAREST
        )

        self.timer = QTimer(self)
        self.timer.timeout.connect(self.cdc_reconnect)

        self.label_fps = QtWidgets.QLabel("FPS: 0")
        self.last_time = QtCore.QTime.currentTime()

        hbox = QtWidgets.QHBoxLayout(self)

        lay = QtWidgets.QVBoxLayout()
        lay.addWidget(self.label_fps)
        lay.addWidget(self.image)

        hbox.addLayout(lay)

        grid = QtWidgets.QGridLayout()
        self.button_up = QtWidgets.QPushButton("Up")
        self.button_up.pressed.connect(lambda: self.send_key(Key.Up))
        self.button_up.released.connect(lambda: self.release_key(Key.Up))
        grid.addWidget(self.button_up, 0, 1)

        self.button_down = QtWidgets.QPushButton("Down")
        self.button_down.pressed.connect(lambda: self.send_key(Key.Down))
        self.button_down.released.connect(lambda: self.release_key(Key.Down))
        grid.addWidget(self.button_down, 2, 1)

        self.button_left = QtWidgets.QPushButton("Left")
        self.button_left.pressed.connect(lambda: self.send_key(Key.Left))
        self.button_left.released.connect(lambda: self.release_key(Key.Left))
        grid.addWidget(self.button_left, 1, 0)

        self.button_right = QtWidgets.QPushButton("Right")
        self.button_right.pressed.connect(lambda: self.send_key(Key.Right))
        self.button_right.released.connect(lambda: self.release_key(Key.Right))
        grid.addWidget(self.button_right, 1, 2)

        self.button_ok = QtWidgets.QPushButton("Ok")
        self.button_ok.pressed.connect(lambda: self.send_key(Key.Ok))
        self.button_ok.released.connect(lambda: self.release_key(Key.Ok))
        grid.addWidget(self.button_ok, 1, 1)

        self.button_back = QtWidgets.QPushButton("Back")
        self.button_back.pressed.connect(lambda: self.send_key(Key.Back))
        self.button_back.released.connect(lambda: self.release_key(Key.Back))
        grid.addWidget(self.button_back, 2, 2)

        hbox.addLayout(grid)

        self.serial = QtSerialPort.QSerialPort(
            PORT,
            baudRate=30_000_000,
            readyRead=self.receive,
        )
        self.serial.errorOccurred.connect(self.cdc_error)
        self.serial.open(QtCore.QIODeviceBase.OpenModeFlag.ReadWrite)

    def keyPressEvent(self, event):
        key = event.key()
        if key == QtCore.Qt.Key.Key_W or key == QtCore.Qt.Key.Key_Up:
            self.send_key(Key.Up)
        elif key == QtCore.Qt.Key.Key_S or key == QtCore.Qt.Key.Key_Down:
            self.send_key(Key.Down)
        elif key == QtCore.Qt.Key.Key_A or key == QtCore.Qt.Key.Key_Left:
            self.send_key(Key.Left)
        elif key == QtCore.Qt.Key.Key_D or key == QtCore.Qt.Key.Key_Right:
            self.send_key(Key.Right)
        elif key == QtCore.Qt.Key.Key_Return or key == QtCore.Qt.Key.Key_Space:
            self.send_key(Key.Ok)
        elif key == QtCore.Qt.Key.Key_Backspace or key == QtCore.Qt.Key.Key_Escape:
            self.send_key(Key.Back)

        event.accept()

    def keyReleaseEvent(self, event):
        key = event.key()
        if key == QtCore.Qt.Key.Key_W or key == QtCore.Qt.Key.Key_Up:
            self.release_key(Key.Up)
        elif key == QtCore.Qt.Key.Key_S or key == QtCore.Qt.Key.Key_Down:
            self.release_key(Key.Down)
        elif key == QtCore.Qt.Key.Key_A or key == QtCore.Qt.Key.Key_Left:
            self.release_key(Key.Left)
        elif key == QtCore.Qt.Key.Key_D or key == QtCore.Qt.Key.Key_Right:
            self.release_key(Key.Right)
        elif key == QtCore.Qt.Key.Key_Return or key == QtCore.Qt.Key.Key_Space:
            self.release_key(Key.Ok)
        elif key == QtCore.Qt.Key.Key_Backspace or key == QtCore.Qt.Key.Key_Escape:
            self.release_key(Key.Back)

        event.accept()

    def send_key(self, key):
        self.serial.write(bytes([key.value | 0b10000000]))

    def release_key(self, key):
        self.serial.write(bytes([key.value]))

    def parse_packet(self):
        FRAME_MAGIC = b"\x55\x14\x69\x88"
        FRAME_SUFFIX = b"\x00\x00\xFF\xF0"
        FRAME_COUNTER_LENGTH = 4
        FRAME_WIDTH = DISPLAY_WIDTH
        FRAME_HEIGHT = DISPLAY_HEIGHT
        FRAME_BYTES_PER_PIXEL = 1
        FRAME_SCREEN_SIZE = FRAME_WIDTH * FRAME_HEIGHT * FRAME_BYTES_PER_PIXEL
        FRAME_TOTAL_SIZE = (
            len(FRAME_MAGIC)
            + FRAME_COUNTER_LENGTH
            + FRAME_SCREEN_SIZE
            + len(FRAME_SUFFIX)
        )
        FRAME_SUFFIX_INDEX = FRAME_TOTAL_SIZE - len(FRAME_SUFFIX)

        while len(self.buffer) >= FRAME_TOTAL_SIZE:
            if (
                self.buffer[:4] == FRAME_MAGIC
                and self.buffer[FRAME_SUFFIX_INDEX:] == FRAME_SUFFIX
            ):
                frame_count = int.from_bytes(
                    self.buffer[
                        len(FRAME_MAGIC) : len(FRAME_MAGIC) + FRAME_COUNTER_LENGTH
                    ],
                    "little",
                )

                if frame_count - self.last_frame_count != 1:
                    print(f"Frame lost: {self.last_frame_count + 1}")
                self.last_frame_count = frame_count

                frame = self.buffer[
                    len(FRAME_MAGIC) + 4 : len(FRAME_MAGIC) + 4 + FRAME_SCREEN_SIZE
                ]
                self.buffer = self.buffer[FRAME_TOTAL_SIZE:]

                # expand frame from RGB332 to RGB888
                frame565 = bytearray()
                for i in range(0, len(frame), 1):
                    byte = frame[i]
                    r = (byte & 0b11100000) >> 5
                    g = (byte & 0b00011100) >> 2
                    b = (byte & 0b00000011) >> 0

                    frame565.append(r * 36)
                    frame565.append(g * 36)
                    frame565.append(b * 85)

                frame = bytes(frame565)

                img = Image.frombytes(
                    "RGB", (DISPLAY_WIDTH, DISPLAY_HEIGHT), frame, "raw", "RGB"
                )

                img = img.resize(
                    (DISPLAY_WIDTH * SCALE, DISPLAY_HEIGHT * SCALE),
                    resample=Image.NEAREST,
                )

                # # add pixel grid
                # for x in range(0, DISPLAY_WIDTH * SCALE, SCALE):
                #     for y in range(0, DISPLAY_HEIGHT * SCALE, SCALE):
                #         img.putpixel((x, y), (0, 0, 0))

                qimage = ImageQt.toqimage(img)
                self.image.setPixmap(QtGui.QPixmap.fromImage(qimage))

                self.image.update()

                # Calculate FPS
                time = QtCore.QTime.currentTime()
                fps = 1_000 / self.last_time.msecsTo(time)
                self.label_fps.setText(f"FPS: {fps:.2f}")
                self.last_time = time
            else:
                self.buffer = self.buffer[1:]

    @QtCore.Slot()
    def receive(self):
        new_data = self.serial.readAll().data()
        if len(new_data) > 0:
            self.buffer += new_data
            self.parse_packet()

    @QtCore.Slot()
    def send(self):
        self.serial.write(self.message_le.text().encode())

    # @QtCore.Slot()
    def cdc_error(self, error):
        if error == QtSerialPort.QSerialPort.SerialPortError.NoError:
            return

        ## launch timer to try to reconnect
        if error == QtSerialPort.QSerialPort.SerialPortError.ResourceError:
            print("Reconnecting...")

            # set image to bars
            img = ImageQt.toqpixmap(self.bars)
            self.image.setPixmap(img)

            self.timer.start(500)

    def cdc_reconnect(self):
        print("Reconnecting...")
        self.serial.close()
        self.serial.open(QtCore.QIODeviceBase.OpenModeFlag.ReadWrite)
        if self.serial.isOpen():
            self.last_frame_count = 0
            self.timer.stop()


if __name__ == "__main__":
    app = QtWidgets.QApplication(sys.argv)
    w = Widget()
    w.show()
    sys.exit(app.exec())
