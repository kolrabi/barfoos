using System;
using Atk;

namespace Editors
{
	public partial class InputTextDialog : Gtk.Dialog
	{
		public string Message 
		{
			get { return this.lblMessage.Text; }
			set { this.lblMessage.Text = value; }
		}

		public string Text 
		{ 
			get { return this.entValue.Text; }
			set { this.entValue.Text = value; } 
		}

		public InputTextDialog ()
		{
			this.Build ();
		}

		protected void OnResponse(object o, Gtk.ResponseArgs args)
		{
			this.Hide ();
		}
	}
}

