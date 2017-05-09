using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Media;
using System.IO;

using VSTHost;

namespace vsthost_tester_csharp
{
    class Tester
    {
        // player
        private SoundPlayer player = new SoundPlayer();

        // wav file
        private const int header_size = 4096;
        private MemoryStream wav = new MemoryStream();
        private MemoryStream wav_clean = new MemoryStream();

        // host
        private const int block_size = 1024;
        private const double sample_rate = 44100.0;
        private HostProxy host = new HostProxy(block_size, sample_rate);
        private Byte[] in_buffer = new Byte[block_size * 2 * 2];
        private Byte[] out_buffer = new Byte[block_size * 2 * 2];

        public void Initialize()
        {
            System.Windows.Forms.MessageBox.Show(Path.GetFullPath("."));
            // load wav from file to clean wav stream
            FileStream wav_file = new FileStream("./../feed/Amen-break.wav", FileMode.Open);
            wav_file.CopyTo(wav_clean);
            wav_file.Close();

            // init player stream
            wav_clean.Position = 0;
            wav_clean.CopyTo(wav);
            wav.Position = 0;
            player.Stream = wav;
            player.Load();
        }

        public void Play()
        {
            player.Play();
        }

        public void Stop()
        {
            player.Stop();
        }

        public void Process()
        {
            MemoryStream wav_processed = new MemoryStream();
            wav_clean.Position = 0;
            wav_clean.CopyTo(wav_processed);
            wav_clean.Position = header_size;
            wav_processed.Position = header_size;

            while (wav_clean.Position < wav_clean.Length)
            {
                // the wav is a 16bit stereo and block_size is in samples
                int temp_block_size_bytes = block_size * 2 * 2;
                // if normal block size would exceed remaining stream length - make it accordingly smaller 
                if (wav_clean.Position + temp_block_size_bytes > wav_clean.Length)
                    temp_block_size_bytes = Convert.ToInt32(wav_clean.Length - wav_clean.Position);

                wav_clean.Read(in_buffer, 0, temp_block_size_bytes);
                host.Process(in_buffer, out_buffer, temp_block_size_bytes / 4);
                wav_processed.Write(out_buffer, 0, temp_block_size_bytes);
            }
            wav_processed.Position = 0;
            player.Stream = wav_processed;
            player.Load();
        }

        public HostControllerProxy GetController()
        {
            return host.GetController();
        }
    }
}
