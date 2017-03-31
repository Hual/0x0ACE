using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Text.RegularExpressions;

namespace _0ace
{
    class Program
    {
        static string challengeUrl;
        static HttpClient client;

        static void Main(string[] args)
        {
            Console.Write("Paste key: ");
            string key = Console.ReadLine();

            client = new HttpClient();
            client.DefaultRequestHeaders.Add("X-0x0ACE-Key", key);

            try
            {
                SolveD34dc0d3();
                Solve0x00000ACE();
            }
            catch (Exception ex)
            {
                Console.WriteLine("Error: {0}", ex.Message);
            }

            Console.ReadKey();
        }

        static async void Solve0x00000ACE()
        {
            try
            {
                string response = await client.GetStringAsync("http://80.233.134.207/0x00000ACE.html");

                Match pathMatch = new Regex("<a href=\"(.*?)\">0x0ACE.BIN<\\/a>").Match(response);

                Console.WriteLine("0x00000ACE challenge URL: {0}", pathMatch.Groups[1].Value);

                challengeUrl = string.Format("http://80.233.134.207{0}", pathMatch.Groups[1].Value);

                HttpResponseMessage download = await client.GetAsync(challengeUrl);

                DownloadContents(download, "0x0ACE.BIN");

                Start0x00000ACEInterpreter("interpreter.exe", "0x0ACE.BIN", pathMatch.Groups[1].Value);
            }
            catch (Exception e)
            {
                Console.WriteLine("0x00000ACE: Exception: {0}", e.Message);
            }
        }

        static void Start0x00000ACEInterpreter(string path, string name, string url)
        {
            Process process = new Process()
            {
                StartInfo = new ProcessStartInfo(path, name)
                {
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true
                }
            };

            process.OutputDataReceived += Interpreter_OutputDataReceived;
            process.ErrorDataReceived += Interpreter_ErrorDataReceived;

            if (!process.Start())
            {
                Console.WriteLine("Could not start 0x00000ACE interpreter.");
                return;
            }

            process.BeginErrorReadLine();
            process.BeginOutputReadLine();
        }

        private static void Interpreter_OutputDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null && e.Data.StartsWith("reg0="))
            {
                Post0x00000ACEResult(e.Data);
            }
        }

        private static void Interpreter_ErrorDataReceived(object sender, DataReceivedEventArgs e)
        {
            if (e.Data != null)
            {
                Console.WriteLine("Could not process assembly: {0}", e.Data);
            }
        }

        static async void Post0x00000ACEResult(string content)
        {
            Console.WriteLine("0x0ACE.BIN output: {0}", content);

            HttpResponseMessage response = await client.PostAsync(challengeUrl, new StringContent(content, Encoding.ASCII, "application/x-www-form-urlencoded"));

            if (!response.IsSuccessStatusCode)
            {
                Console.WriteLine("0x00000ACE: Server returned error code {0}.", response.StatusCode);
                return;
            }

            DownloadContents(response, "0x00000ACE.out");
        }

        static async void SolveD34dc0d3()
        {
            try
            {
                string response = await client.GetStringAsync("http://5.9.247.121/d34dc0d3");

                Match numbersMatch = new Regex(@"\[([0-9]+), \.\.\., ([0-9]+)\]").Match(response);
                int min = int.Parse(numbersMatch.Groups[1].Value);
                int max = int.Parse(numbersMatch.Groups[2].Value);

                Match secretMatch = new Regex("\\<input type=\"hidden\" name=\"verification\" value=\"(.*?)\" \\/\\>").Match(response);
                string secret = secretMatch.Groups[1].Value;

                StringBuilder numbers = new StringBuilder(string.Format("verification={0}&solution=", Uri.EscapeDataString(secret.ToString())));
                bool[] primes = GeneratePrimes(max);

                string delimiter = Uri.EscapeDataString(",");

                for (int i = min + 2; i < max - 2; i += 2)
                {
                    if (primes[i])
                    {
                        numbers.AppendFormat("{0}{1}", i, delimiter);
                    }
                }

                numbers.Length -= delimiter.Length;

                HttpResponseMessage responseFinal = await client.PostAsync("http://5.9.247.121/d34dc0d3", new StringContent(numbers.ToString(), Encoding.ASCII, "application/x-www-form-urlencoded"));

                if (!responseFinal.IsSuccessStatusCode)
                {
                    Console.WriteLine("d34dc0d3: Server returned error code {0}.", responseFinal.StatusCode);
                    return;
                }

                DownloadContents(responseFinal, "d34dc0d3.out");
            }
            catch (Exception e)
            {
                Console.WriteLine("d34dc0d3: Exception: {0}", e.Message);
            }
        }

        static async void DownloadContents(HttpResponseMessage response, string path)
        {
            Stream stream = await response.Content.ReadAsStreamAsync();
            FileStream streamOut = File.Create(path);

            using (StreamReader reader = new StreamReader(stream))
            {
                stream.CopyTo(streamOut);
                streamOut.Flush();
            }

            streamOut.Close();
            stream.Close();
            Console.WriteLine("Downloaded file: {0}", path);
        }

        static bool[] GeneratePrimes(int max)
        {
            bool[] output = Enumerable.Repeat(true, max).ToArray();

            for (int i = 2; i < Math.Sqrt(max); ++i)
            {
                if (output[i])
                {
                    for (int j = i*i; j < max; j += i)
                    {
                        output[j] = false;
                    }
                }
            }

            return output;
        }
    }
}
