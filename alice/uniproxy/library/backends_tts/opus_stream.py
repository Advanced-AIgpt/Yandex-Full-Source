import asyncio
import datetime
import struct
import time
import tornado.gen

from tornado.ioloop import IOLoop


class PacketReader:
    def __init__(self, packet):
        self.packet = packet
        self.pos = 0

    def skip(self, count_bytes, msg):
        new_pos = self.pos + count_bytes
        if new_pos > len(self.packet):
            raise Exception('read <{}> out of packet size={}'.format(msg, len(self.packet)))

        self.pos = new_pos

    def read_val(self, fmt, val_size, msg):
        new_pos = self.pos + val_size
        if new_pos > len(self.packet):
            raise Exception('read <{}> (size={}) out of packet size={}'.format(msg, val_size, len(self.packet)))

        value = struct.unpack(fmt, self.packet[self.pos:new_pos])[0]
        self.pos = new_pos
        return value

    def read_str(self, size, msg, coding='utf-8'):
        new_pos = self.pos + size
        if new_pos > len(self.packet):
            raise Exception('read <{}> out of packet size={}'.format(msg, len(self.packet)))

        s = self.packet[self.pos:new_pos].decode('utf-8', errors='ignore')
        self.pos = new_pos
        return s

    def read_u8(self, msg):
        return self.read_val('<B', 1, msg)

    def read_u16(self, msg):
        return self.read_val('<H', 2, msg)

    def read_u32(self, msg):
        return self.read_val('<L', 4, msg)

    def read_u64(self, msg):
        return self.read_val('<Q', 8, msg)

    def read_s8(self, msg):
        return self.read_val('<b', 1, msg)

    def read_s16(self, msg):
        return self.read_val('<h', 2, msg)

    def read_s32(self, msg):
        return self.read_val('<l', 4, msg)

    def read_s64(self, msg):
        return self.read_val('<q', 8, msg)


class OggStream:
    """ RFC 3533
    """
    HEADER_SIZE = 27
    HEADER_FORMAT = '<LBBQLLLB'
    FLAG_CONTINUATION = 1
    CONTINUE_SEGMENT_SIZE = 255

    def __init__(self, stream):
        """ stream MUST have async read(self, size) method
        """
        self.stream = stream
        self.expect_continuation = False
        self.cont_packet = b''

    async def process(self):
        while True:
            ogg_header = await self.stream.read(OggStream.HEADER_SIZE)
            if ogg_header is None or len(ogg_header) == 0:
                break   # TODO: check EOS flag?

            if len(ogg_header) != OggStream.HEADER_SIZE:
                raise Exception('invalid ogg header size')

            signature, version, flags, granule_position, serial_number, \
                sequence_number, checksum, total_segments = struct.unpack(OggStream.HEADER_FORMAT, ogg_header)
            if self.expect_continuation and (flags & OggStream.FLAG_CONTINUATION) != 0:
                raise Exception('expect ogg header with continuation flag')

            self.expect_continuation = False
            segments_table = await self.stream.read(total_segments)
            if segments_table is None:
                raise Exception('got EOF when expect ogg segments table')

            if len(segments_table) != total_segments:
                raise Exception('invalid ogg segment table size')

            segments_sizes = struct.unpack('{}B'.format(total_segments), segments_table)
            packet_size = 0
            segments_data = b''
            for ssize in segments_sizes:
                packet_size += ssize
                if ssize == OggStream.CONTINUE_SEGMENT_SIZE:
                    continue

                packet = await self.stream.read(packet_size)
                if packet is None:
                    raise Exception('got EOF when expect ogg packet content')

                if len(packet) != packet_size:
                    raise Exception('fail read ogg segmens (total size={})'.format(packet_size))

                packet_size = 0
                if len(self.cont_packet):
                    packet = self.cont_packet + packet
                    self.cont_packet = b''

                await self.on_read_packet(packet)
                segments_data += packet

            await self.on_read_ogg_page(ogg_header + segments_table + segments_data)

            if packet_size != 0:
                self.expect_continuation = True
                self.cont_packet = await self.stream.read(packet_size)
                if len(self.cont_packet) != packet_size:
                    raise Exception('fail read not full (cont.) packet size={}'.format(packet_size))

    async def on_read_packet(self, packet):
        pass

    async def on_read_ogg_page(self, page):
        pass


class IdentificationHeader:
    """ RFC 7845
    """
    def __init__(self, packet_reader):
        packet_reader.skip(8, 'Magic Signature OpusHeader')
        self.version = packet_reader.read_u8('Version')
        self.channel_count = packet_reader.read_u8('Channel count')
        self.pre_skip = packet_reader.read_u16('Pre-skip')
        self.input_sample_rate = packet_reader.read_u32('Input sample rate')
        self.opus_gain = packet_reader.read_s16('Opus gain')
        self.channel_mapping_family = packet_reader.read_u8('Channel mapping family')
        # TODO: read Mapping Table... ?

    def __str__(self):
        return 'version={} channel_count={} pre_skip={} input_sample_rate={} opus_gain={}'.format(
            self.version,
            self.channel_count,
            self.pre_skip,
            self.input_sample_rate,
            self.opus_gain,
        )

    @staticmethod
    def in_packet(packet):
        return len(packet) >= 19 and packet[:8] == b'OpusHead'


class CommentHeader:
    """ RFC 7845
    """
    def __init__(self, packet_reader):
        packet_reader.skip(8, 'Magic signature OpusTags')
        vendor_string_length = packet_reader.read_u32('Vendor string length')
        self.vendor = packet_reader.read_str(vendor_string_length, 'Vendor string')
        user_comment_list_length = packet_reader.read_u32('User comment list length')
        self.comments = []
        for i in range(user_comment_list_length):
            comment_length = packet_reader.read_u32('Comment length')
            self.comments.append(packet_reader.read_str(comment_length, 'Comment string'))

    def __str__(self):
        result = 'vendor={}'.format(self.vendor)
        if self.comments:
            result += '\ncomments[{}]:\n\t'.format(len(self.comments)) + '\n\t'.join(self.comments)
        return result

    @staticmethod
    def in_packet(packet):
        return len(packet) >= 8 and packet[:8] == b'OpusTags'


class OpusPacket:
    """ RFC 6716
    """
    FRAME_SIZE_FOR_CONF_NUMBER = [  # milliseconds
        10, 20, 40, 60,
        10, 20, 40, 60,
        10, 20, 40, 60,
        10, 20,
        10, 20,
        2.5, 5, 10, 20,
        2.5, 5, 10, 20,
        2.5, 5, 10, 20,
        2.5, 5, 10, 20,
    ]
    TOC_STEREO_BITMASK = 4
    NUMBER_OF_FRAMES_BITMASK = 63
    PADDING_BITMASK = 64
    VBR_BITMASK = 128
    FRAMES_PER_PACKET_CODE_BITMASK = 3
    FRAMES_PER_PACKET_CODE0 = 0
    FRAMES_PER_PACKET_CODE1 = 1
    FRAMES_PER_PACKET_CODE2 = 2
    FRAMES_PER_PACKET_CODE3 = 3

    def __init__(self, packet_reader):
        # RFC 6716 section 3.1. The TOC Byte
        toc_byte = packet_reader.read_u8('TOC byte')
        cfg_num = toc_byte >> 3
        self.frame_size = OpusPacket.FRAME_SIZE_FOR_CONF_NUMBER[cfg_num]
        self.stereo = bool(toc_byte & OpusPacket.TOC_STEREO_BITMASK)
        self.frames_per_packet_code = toc_byte & OpusPacket.FRAMES_PER_PACKET_CODE_BITMASK
        self.vbr = False
        if self.frames_per_packet_code == OpusPacket.FRAMES_PER_PACKET_CODE0:
            self.frames_count = 1
        elif self.frames_per_packet_code == OpusPacket.FRAMES_PER_PACKET_CODE1:
            self.frames_count = 2
        elif self.frames_per_packet_code == OpusPacket.FRAMES_PER_PACKET_CODE2:
            self.frames_count = 2
            self.vbr = True
        elif self.frames_per_packet_code == OpusPacket.FRAMES_PER_PACKET_CODE3:
            # TODO: not tested branch (need audio example)
            frame_count_byte = packet_reader.read_u8('Frame count byte')
            self.frames_count = OpusPacket.NUMBER_OF_FRAMES_BITMASK & frame_count_byte
            self.vbr = bool(OpusPacket.VBR_BITMASK & frame_count_byte)
            self.padding = bool(OpusPacket.PADDING_BITMASK & frame_count_byte)

    def duration(self):
        return self.frame_size * self.frames_count

    def __str__(self):
        return 'frame_size={} stereo={} vbr={} fpp_code={} frames_count={}'.format(
            self.frame_size,
            self.stereo,
            self.vbr,
            self.frames_per_packet_code,
            self.frames_count,
        )


class OpusStream(OggStream):
    """ RFC 6716
    """
    def __init__(self, stream):
        OggStream.__init__(self, stream)
        self.identification_header = None
        self.comment_headers = []
        self.duration = 0

    async def on_read_packet(self, packet):
        packet_reader = PacketReader(packet)
        if IdentificationHeader.in_packet(packet):
            identification_header = IdentificationHeader(packet_reader)
            await self.on_identification_header(identification_header, packet)
            if self.identification_header is None:
                identification_header = self.identification_header
            return

        if CommentHeader.in_packet(packet):
            comment_header = CommentHeader(packet_reader)
            await self.on_comment_header(comment_header, packet)
            self.comment_headers.append(comment_header)
            return

        opus_packet = OpusPacket(packet_reader)
        await self.on_opus_packet(opus_packet, packet)
        self.duration += opus_packet.duration()

    async def on_identification_header(self, hdr, packet):
        pass

    async def on_comment_header(self, hdr, packet):
        pass

    async def on_opus_packet(self, opus_packet, packet):
        pass


class OpusStreamPrinter(OpusStream):
    """ for debug purpouse
    """
    async def on_identification_header(self, hdr, packet):
        print(hdr)

    async def on_comment_header(self, hdr, packet):
        print(hdr)

    async def on_opus_packet(self, opus_packet, packet):
        print(opus_packet)


class RealtimeOpusStream(OpusStream):
    def __init__(self, input_stream, output_stream, jitter_buffer=0):
        """ send first jitter_buffer=seconds audio immediately (jitter buffer for receiver)
            input_stream MUST have async read(self, size) method
            output_stream MUST have async write(self, data) method
        """
        super().__init__(input_stream)
        self.output_stream = output_stream
        self.jitter_buffer_duration = jitter_buffer
        self.start_stream_time = None
        self.total_duration = 0

    async def on_opus_packet(self, opus_packet, packet):
        if self.start_stream_time is None:
            self.start_stream_time = time.time()
        self.total_duration += opus_packet.duration() / 1000.

    async def on_read_ogg_page(self, page):
        await self.output_stream.write(page)
        if self.start_stream_time is None:
            # not delay non-audio pages (header)
            return

        real_duration = time.time() - self.start_stream_time
        expected_duration = self.total_duration - self.jitter_buffer_duration
        sleep_for = expected_duration - real_duration  # seconds
        if sleep_for > 0.05:
            await tornado.gen.sleep(sleep_for)

# ============ debug section ==============


class AsyncAdapter:
    def __init__(self, stream):
        self.stream = stream

    async def read(self, size):
        return self.stream.read(size)


class TestOutput:
    def __init__(self, stream):
        self.stream = stream

    async def write(self, data):
        self.stream.write(data)
        print('{} write(size={})'.format(datetime.datetime.now().isoformat(), len(data)))


if __name__ == "__main__":
    # for local testin/debug this file
    with open('24057262_Gav_Seredina_sosiski.opus', 'rb') as f, open('opus_stream_realtime.opus', 'wb') as fo:
        printer = False
        if printer:
            loop = asyncio.get_event_loop()
            opus_stream = OpusStreamPrinter(AsyncAdapter(f))
            # blocking call which returns when the opus_stream.process() coroutine is done
            loop.run_until_complete(opus_stream.process())
            loop.close()
        else:
            start_time = time.time()
            jitter_buffer = 5
            opus_stream = RealtimeOpusStream(AsyncAdapter(f), TestOutput(fo), jitter_buffer=jitter_buffer)
            IOLoop.current().run_sync(opus_stream.process)
            print('real streaming duration+jitter_buffer={} ()'.format(
                round(time.time() - start_time + jitter_buffer, 3)))
        print('calculated from frames duration={}'.format(round(opus_stream.duration / 1000., 3)))
