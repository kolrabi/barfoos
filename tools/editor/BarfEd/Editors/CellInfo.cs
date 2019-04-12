using System;
using System.IO;

namespace Editors
{
	public class CellInfo
	{
		public readonly string Name, Path;

		public int Flags = CellFlags.Solid;


		public const int TagEnd 	= 0xFFAA;
		public const int TagFlags	= 0xF001;


		public CellInfo (string name, string path)
		{
			this.Name = name;
			this.Path = path;

			this.Load ();
		}

		private void Load()
		{
			using (TagReader reader = new TagReader (this.Path)) {

				int tag = reader.ReadTag ();
				while (tag != TagEnd && !reader.Eof) {
					switch (tag) {
						case TagFlags:

					}
					tag = reader.ReadTag ();
				}
			}
		}

		public void Save() 
		{
			using (TagWriter writer = new TagWriter (this.Path)) {
				writer.WriteTag (TagFlags, this.Flags);
				writer.WriteTag (TagEnd);
			}
		}
	}
}

