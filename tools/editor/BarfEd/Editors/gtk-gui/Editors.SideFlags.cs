
// This file has been generated by the GUI designer. Do not modify.
namespace Editors
{
	public partial class SideFlags
	{
		private global::Gtk.HBox hbox3;
		
		private global::Gtk.ToggleButton togglebutton1;
		
		private global::Gtk.ToggleButton togglebutton2;
		
		private global::Gtk.ToggleButton togglebutton3;
		
		private global::Gtk.ToggleButton togglebutton4;
		
		private global::Gtk.ToggleButton togglebutton5;
		
		private global::Gtk.ToggleButton togglebutton6;

		protected virtual void Build ()
		{
			global::Stetic.Gui.Initialize (this);
			// Widget Editors.SideFlags
			global::Stetic.BinContainer.Attach (this);
			this.Name = "Editors.SideFlags";
			// Container child Editors.SideFlags.Gtk.Container+ContainerChild
			this.hbox3 = new global::Gtk.HBox ();
			this.hbox3.Name = "hbox3";
			this.hbox3.Spacing = 6;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton1 = new global::Gtk.ToggleButton ();
			this.togglebutton1.WidthRequest = 24;
			this.togglebutton1.HeightRequest = 24;
			this.togglebutton1.CanFocus = true;
			this.togglebutton1.Name = "togglebutton1";
			this.togglebutton1.UseUnderline = true;
			this.togglebutton1.FocusOnClick = false;
			this.togglebutton1.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton1.Label = global::Mono.Unix.Catalog.GetString ("L");
			this.hbox3.Add (this.togglebutton1);
			global::Gtk.Box.BoxChild w1 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton1]));
			w1.Position = 0;
			w1.Expand = false;
			w1.Fill = false;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton2 = new global::Gtk.ToggleButton ();
			this.togglebutton2.WidthRequest = 24;
			this.togglebutton2.HeightRequest = 24;
			this.togglebutton2.CanFocus = true;
			this.togglebutton2.Name = "togglebutton2";
			this.togglebutton2.UseUnderline = true;
			this.togglebutton2.FocusOnClick = false;
			this.togglebutton2.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton2.Label = global::Mono.Unix.Catalog.GetString ("R");
			this.hbox3.Add (this.togglebutton2);
			global::Gtk.Box.BoxChild w2 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton2]));
			w2.Position = 1;
			w2.Expand = false;
			w2.Fill = false;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton3 = new global::Gtk.ToggleButton ();
			this.togglebutton3.WidthRequest = 24;
			this.togglebutton3.HeightRequest = 24;
			this.togglebutton3.CanFocus = true;
			this.togglebutton3.Name = "togglebutton3";
			this.togglebutton3.UseUnderline = true;
			this.togglebutton3.FocusOnClick = false;
			this.togglebutton3.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton3.Label = global::Mono.Unix.Catalog.GetString ("U");
			this.hbox3.Add (this.togglebutton3);
			global::Gtk.Box.BoxChild w3 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton3]));
			w3.Position = 2;
			w3.Expand = false;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton4 = new global::Gtk.ToggleButton ();
			this.togglebutton4.WidthRequest = 24;
			this.togglebutton4.HeightRequest = 24;
			this.togglebutton4.CanFocus = true;
			this.togglebutton4.Name = "togglebutton4";
			this.togglebutton4.UseUnderline = true;
			this.togglebutton4.FocusOnClick = false;
			this.togglebutton4.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton4.Label = global::Mono.Unix.Catalog.GetString ("D");
			this.hbox3.Add (this.togglebutton4);
			global::Gtk.Box.BoxChild w4 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton4]));
			w4.Position = 3;
			w4.Expand = false;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton5 = new global::Gtk.ToggleButton ();
			this.togglebutton5.WidthRequest = 24;
			this.togglebutton5.HeightRequest = 24;
			this.togglebutton5.CanFocus = true;
			this.togglebutton5.Name = "togglebutton5";
			this.togglebutton5.UseUnderline = true;
			this.togglebutton5.FocusOnClick = false;
			this.togglebutton5.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton5.Label = global::Mono.Unix.Catalog.GetString ("F");
			this.hbox3.Add (this.togglebutton5);
			global::Gtk.Box.BoxChild w5 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton5]));
			w5.Position = 4;
			w5.Expand = false;
			// Container child hbox3.Gtk.Box+BoxChild
			this.togglebutton6 = new global::Gtk.ToggleButton ();
			this.togglebutton6.WidthRequest = 24;
			this.togglebutton6.HeightRequest = 24;
			this.togglebutton6.CanFocus = true;
			this.togglebutton6.Name = "togglebutton6";
			this.togglebutton6.UseUnderline = true;
			this.togglebutton6.FocusOnClick = false;
			this.togglebutton6.Relief = ((global::Gtk.ReliefStyle)(2));
			this.togglebutton6.Label = global::Mono.Unix.Catalog.GetString ("B");
			this.hbox3.Add (this.togglebutton6);
			global::Gtk.Box.BoxChild w6 = ((global::Gtk.Box.BoxChild)(this.hbox3 [this.togglebutton6]));
			w6.Position = 5;
			w6.Expand = false;
			w6.Fill = false;
			this.Add (this.hbox3);
			if ((this.Child != null)) {
				this.Child.ShowAll ();
			}
			this.Hide ();
		}
	}
}
