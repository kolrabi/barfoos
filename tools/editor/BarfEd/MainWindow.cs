using System;
using Gtk;

public partial class MainWindow: Gtk.Window
{
	public MainWindow (string path) : base (Gtk.WindowType.Toplevel)
	{
		Build ();

		this.celleditor1.AssetPath = path;
	}

	protected void OnDeleteEvent (object sender, DeleteEventArgs a)
	{
		Application.Quit ();
		a.RetVal = true;
	}
}
