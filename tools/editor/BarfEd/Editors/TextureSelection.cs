using System;

namespace Editors
{
	[System.ComponentModel.ToolboxItem (true)]
	public partial class TextureSelection : Gtk.Bin
	{
		public string Title 
		{
			set { this.lblText.Text = value; }
			get { return this.lblText.Text; }
		}

		public TextureSelection ()
		{
			this.Build ();
		}
	}
}

