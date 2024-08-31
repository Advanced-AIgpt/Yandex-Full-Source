import struct
import tempfile
import wave
import voicetech.asr.core.lib.util.io.byte_stream_converter.py_lib.lib as converter_lib

def opus_to_wav(opus):
    try:
        converter = converter_lib.get_opus_converter()
        wav_data = converter.add_chunk(opus)
        wav_data += converter.finish()
        return converter_lib.waveform_to_wav(wav_data)
    except RuntimeError:
        return None

def norm(wav):
    with tempfile.NamedTemporaryFile() as tmp:
        tmp.write(wav)
        tmp.flush()

        waveobj = wave.open(tmp.name, 'rb')
        rate = waveobj.getframerate()
        assert rate == 16000, rate
        width = waveobj.getsampwidth()
        assert width == 2
        frames = waveobj.readframes(rate*100000)
        params = waveobj.getparams()
        waveobj.close()

        fmt = 'h' * (len(frames) // width)
        values = struct.unpack(fmt, frames)
        amp = (max(values) - min(values))
        mid = (max(values) + min(values)) / 2
        values = [int((value - mid)/float(amp) * 2 * 30000) for value in values]
        frames = struct.pack(fmt, *values)

        assert len(frames) % width == 0

        waveobj = wave.open(tmp.name, 'wb')
        waveobj.setparams(params)
        waveobj.writeframes(frames)

        tmp.flush()
        tmp.seek(0)
        return tmp.read()

