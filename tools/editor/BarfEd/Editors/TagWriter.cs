using System;
using System.IO;
using System.Text;

namespace Editors
{
	public class TagWriter : IDisposable
	{
		readonly FileStream stream;

		public TagWriter (string path)
		{
			this.stream = File.OpenWrite (path);
		}

		#region IDisposable implementation
		void IDisposable.Dispose ()
		{
			this.stream.Dispose ();
		}
		#endregion

		public void Write(byte b)
		{
			this.stream.WriteByte (b);
		}

		public void Write(short s) 
		{
			this.stream.WriteByte ((byte)((s   ) & 0xff));
			this.stream.WriteByte ((byte)((s>>8) & 0xff));
		}

		public void Write(int i) 
		{
			this.Write((short)((i    ) & 0xffff));
			this.Write((short)((i>>16) & 0xffff));
		}

		public void Write(string s)
		{
			byte[] bytes = Encoding.UTF8.GetBytes (s);
			this.Write (bytes.Length);
			this.stream.Write (bytes, 0, bytes.Length);
		}

		public void WriteTag(int id)
		{
			this.Write ((short)id);
		}

		public void WriteTag(int id, int v)
		{
			this.Write ((short)id);
			this.Write (v);
		}
	}
}

