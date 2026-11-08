#!/usr/bin/env python3
"""Place genuine USB recordings inside the supplied Cardputer ADV photograph."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def audio_hash(ffmpeg,path):
    return subprocess.check_output([ffmpeg,'-hide_banner','-loglevel','error','-nostdin',
        '-i',str(path),'-map','0:a:0','-c:a','copy','-f','hash','-hash','sha256','-']).decode().strip().split('=',1)[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('fernando',type=Path)
    parser.add_argument('ole_toro',type=Path)
    parser.add_argument('--ffmpeg')
    args=parser.parse_args()
    ffmpeg=args.ffmpeg or shutil.which('ffmpeg')
    if not ffmpeg:
        import imageio_ffmpeg
        ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
    template=ROOT/'docs/images/cardputer-adv-template.png'
    header=template.read_bytes()[:24]
    assert header[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',header[16:24])==(1200,1200)
    folder=ROOT/'docs/media';folder.mkdir(exist_ok=True)
    for game,source,name,poster_at in (
        ('Fernando Martín',args.fernando,'fernando-martin',180),
        ('Olé Toro',args.ole_toro,'ole-toro',343)):
        target=folder/(name+'-cardputer-adv.mp4')
        if target.exists(): raise FileExistsError(target)
        # Keep the original photograph. Only its white margins are cropped.
        # The actual 240x135 picture is placed inside the LCD at 1:1 pixels.
        subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-nostdin',
            '-i',str(source),'-loop','1','-framerate','25','-i',str(template),
            '-filter_complex','[1:v]crop=880:580:160:310[body];'
                '[0:v]scale=240:135:flags=neighbor[screen];'
                '[body][screen]overlay=231:54:shortest=1[video]',
            '-map','[video]','-map','0:a:0','-c:v','libx264','-preset','fast',
            '-crf','16','-pix_fmt','yuv420p','-c:a','copy',
            '-movflags','+faststart',str(target)],check=True)
        subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-nostdin',
            '-ss',str(poster_at),'-i',str(target),'-frames:v','1','-update','1',
            str(folder/(name+'-poster.png'))],check=True)
        original=json.loads(source.with_suffix('.json').read_text())
        assert original['full_tape_unabridged']
        original_audio=audio_hash(ffmpeg,source)
        assert audio_hash(ffmpeg,target)==original_audio,'Audio packets differ from the original'
        metadata={key:original[key] for key in ('seconds','frames','video_frames',
            'protocol','timing','ending','full_tape_unabridged')}
        metadata.update(game=game,video=target.name,captured_firmware=original['firmware'],
            source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
            sha256=hashlib.sha256(target.read_bytes()).hexdigest(),
            photograph='User-supplied Cardputer ADV photograph; M5Stack product image',
            photograph_source='https://docs.m5stack.com/en/core/Cardputer-Adv',
            photograph_sha256=hashlib.sha256(template.read_bytes()).hexdigest(),
            canvas=[880,580],screen_rect=[231,54,240,135],audio='Original AAC stream copied without re-encoding',
            audio_packets_sha256=original_audio)
        target.with_suffix('.json').write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n')
        print(target.name,metadata['sha256'],flush=True)

if __name__=='__main__':main()
