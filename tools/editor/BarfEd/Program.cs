using System;
using Gtk;

namespace BarfEd
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			Application.Init ();

			FileChooserDialog dlg = new FileChooserDialog ("title", null, FileChooserAction.SelectFolder, "Ok", ResponseType.Ok, "Cancel", ResponseType.Cancel);
			if (dlg.Run () == (int)ResponseType.Ok) {
				dlg.Hide ();
				MainWindow win = new MainWindow (dlg.Filename);
				win.Show ();
				Application.Run ();
			}
		}
	}
}
