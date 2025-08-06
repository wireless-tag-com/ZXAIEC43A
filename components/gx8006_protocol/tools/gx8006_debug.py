import queue
import struct
import threading
import time
import os
import wave
import opuslib
from datetime import datetime
import serial
from loguru import logger

# pip install opuslib && pip install loguru && pip install wave

class SerialProtocol:
    FRAME_MIN_LEN = 8
    PACK_FIRST_BYTE = 0x55  # 匹配C代码
    PACK_SECOND_BYTE = 0xAA  # 匹配C代码

    def __init__(self, port="COM54", baudrate=9600):
        self.ser = serial.Serial(port, baudrate, timeout=1, rtscts=False)  # 匹配C代码的无流控
        self.running = True
        self.read_thread = threading.Thread(target=self._read_thread, daemon=True)
        self.rx_buffer = bytearray()
        self.rx_queue = queue.Queue()
        self.lock = threading.Lock()
        self.read_thread.start()
        logger.info(f"SerialProtocol initialized on port={port}, baudrate={baudrate}")

    @staticmethod
    def calculate_checksum(data):
        # 匹配C代码的简单累加和算法
        checksum = 0
        for byte in data:
            checksum += byte
        return checksum & 0xFF

    def build_frame(self, cmd, payload):
        payload_len = len(payload)
        # 匹配C代码的帧结构: [0x55][0xAA][0x00][0x92][len_high][len_low][cmd][payload][checksum]
        header = struct.pack('BBBB', self.PACK_FIRST_BYTE, self.PACK_SECOND_BYTE, 0x00, 0x92)
        # 长度字段表示cmd+payload的长度
        length_field = struct.pack('>H', payload_len + 1)
        cmd_byte = struct.pack('B', cmd)

        # 计算校验和（除了最后的校验和字节）
        frame_without_checksum = header + length_field + cmd_byte + payload
        checksum = self.calculate_checksum(frame_without_checksum)
        checksum_byte = struct.pack('B', checksum)

        return frame_without_checksum + checksum_byte

    def write(self, cmd, payload):
        frame = self.build_frame(cmd, payload)
        self.ser.write(frame)
        logger.info(f"Sent frame with CMD=0x{cmd:02x}, length={len(frame)}")

    def _read_thread(self):
        while self.running:
            try:
                data = self.ser.read(self.ser.in_waiting or 1)
                if data:
                    with self.lock:
                        self.rx_buffer.extend(data)
                        self._process_buffer()
            except serial.SerialException as e:
                logger.error(f"Serial exception: {e}")
                self.running = False
            time.sleep(0.001)

    def _process_buffer(self):
        while len(self.rx_buffer) >= self.FRAME_MIN_LEN:
            # 查找帧头
            if self.rx_buffer[0] != self.PACK_FIRST_BYTE or self.rx_buffer[1] != self.PACK_SECOND_BYTE:
                self.rx_buffer.pop(0)
                continue

            if len(self.rx_buffer) < 6:
                break

            # 检查固定字节
            if self.rx_buffer[3] != 0x92:
                logger.warning("Invalid fixed bytes, dropping data")
                self.rx_buffer.pop(0)
                continue

            # 获取长度字段（cmd + payload的长度）
            cmd_payload_len = struct.unpack('>H', self.rx_buffer[4:6])[0]
            frame_len = self.FRAME_MIN_LEN + cmd_payload_len - 1  # 总帧长度

            if frame_len > 61 * 1024 or frame_len < self.FRAME_MIN_LEN:
                logger.warning(f"Invalid frame length {frame_len}, dropping data")
                self.rx_buffer.pop(0)
                continue

            if len(self.rx_buffer) < frame_len:
                break

            frame = self.rx_buffer[:frame_len]
            self.rx_buffer = self.rx_buffer[frame_len:]

            # 验证校验和
            received_checksum = frame[-1]
            calculated_checksum = self.calculate_checksum(frame[:-1])

            if received_checksum != calculated_checksum:
                logger.error(
                    f"Checksum mismatch, dropping frame, received: 0x{received_checksum:02x}, calculated: 0x{calculated_checksum:02x}")
                continue

            # 提取cmd和payload
            cmd = frame[6]
            payload = frame[7:-1]  # 从cmd后到校验和前

            self.rx_queue.put((cmd, payload))
            logger.debug(f"Enqueued frame with CMD=0x{cmd:02x}, Payload length={len(payload)}")

    def async_read(self, timeout=None):
        try:
            cmd, payload = self.rx_queue.get(timeout=timeout)
            return cmd, payload
        except queue.Empty:
            return None, None

    def frame_received(self, cmd, payload):
        logger.debug(f"Received CMD=0x{cmd:02x}, Payload={payload[:100]}")

    def close(self):
        self.running = False
        self.read_thread.join()
        self.ser.close()
        logger.info("SerialProtocol closed")

class OpusToWavWriter:
    def __init__(self, wav_path, sample_rate=16000, channels=1, sample_width=2):
        self.wav_file = wave.open(wav_path, 'wb')
        self.wav_file.setnchannels(channels)
        self.wav_file.setsampwidth(sample_width)
        self.wav_file.setframerate(sample_rate)
        self.decoder = opuslib.Decoder(sample_rate, channels)
        self.frame_size = int(sample_rate * 40 / 1000)

    def write_opus(self, opus_bytes):
        decoded = self.decoder.decode(opus_bytes, self.frame_size)
        self.wav_file.writeframes(decoded)

    def close(self):
        self.wav_file.close()

if __name__ == "__main__":
    protocol = SerialProtocol(port="/dev/ttyUSB0", baudrate=921600)
    os.makedirs("files", exist_ok=True)
    opus_decoder = None

    while True:
        cmd, payload = protocol.async_read(1000000)
        if cmd == 0x05:
            status = payload[0]
            if status == 0x00:
                logger.info("start ----------------->")
                time_now = datetime.now().strftime('%Y%m%dT%H%M%SZ')
                opus_decoder = OpusToWavWriter(f"files/gx8006_opus_{time_now}.wav")
            elif status == 0x01:
                if opus_decoder is not None:
                    opus_decoder.write_opus(bytes(payload[1:]))
            elif status == 0x02:
                logger.info("-----------------------> finish ")
                opus_decoder.close()
                opus_decoder = None
