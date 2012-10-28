using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO.Ports;
using System.IO;

namespace TestSerialPort
{
	class Program
	{
		// DIRECTIONS:
		//	Assemble VC4 program to bytecode, set path here.
		//	(No need to pad beginning with 512 bytes as with bootcode.bin)
		private const string PATH = @"C:\dev\vc4\assembler\prog.bin";
		//	Use included loader.bin for the Pi
		//	run this program, put a breakpoint at the end and use the debugger to see what data is returned.

		// NOTE - this should compile with any C# compiler, but the serial port stuff is Windows specific.
		// I use an FT232-based USB adapter with the Ground/Tx/Rx pins hooked up to the Pi, and the port set to COM7.
		// Please feel free to write something easier to use!!
		
		static void Main(string[] args)
		{
			var sp = new SerialPort("COM7", 115200, Parity.None, 8);
			sp.Open();

			byte[] data;
			using (var sr = new BinaryReader(new FileStream(PATH, FileMode.Open, FileAccess.Read)))
			{
				int length = (int)sr.BaseStream.Length;
				sr.BaseStream.Position = 0;

				data = new byte[length + 5];
				// 0x7C indicates start of data to send
				data[0] = 0x7C;
				// size of data to send, 32-bit, big endian
				data[4] = (byte)(length & 0xFF);
				data[3] = (byte)((length >> 8) & 0xFF);
				data[2] = (byte)((length >> 16) & 0xFF);
				data[1] = (byte)((length >> 24) & 0xFF);

				// data to send
				sr.Read(data, 5, length);
			}
			
			sp.Write(data, 0, data.Length);

			// size of data to receive, 32-bit, big endian
			int recvLength = 0;
			for (int i = 0; i < 4; i++)
			{
				recvLength = (recvLength << 8) | (byte)sp.ReadByte();
			}

			// data to receive
			byte[] recvData = new byte[recvLength];
			for (int i = 0; i < recvData.Length; i++)
			{
				recvData[i] = (byte)sp.ReadByte();
			}
			
			// PUT BREAKPOINT HERE
		}
	}
}
