using System;
using Gtk;
using System.IO;
using Atk;

namespace Editors
{

	[Gtk.TreeNode (ListOnly=true)]
	public class CellNode : Gtk.TreeNode {
		[Gtk.TreeNodeValue (Column=0)]
		public string Name;

		[Gtk.TreeNodeValue (Column=1)]
		public string Path;

		public CellNode (string path)
		{
			this.Path = path;
			this.Name = System.IO.Path.GetFileNameWithoutExtension (path);
		}
	}

	[System.ComponentModel.ToolboxItem (true)]
	public partial class CellEditor : Gtk.Bin
	{
		readonly Gtk.NodeStore CellStore;

		private string assetPath = "";
		public string AssetPath 
		{
			get { return assetPath; }
			set {
				this.assetPath = value;
				this.UpdateCellList ();
			}
		}

		private CellInfo cell = null;
		public CellInfo Cell 
		{
			get { return this.cell; }
			set {
				this.cell = value;
				this.UpdateCell ();
			}
		}

		public CellEditor ()
		{
			this.Build ();
			this.CellStore = new Gtk.NodeStore (typeof (CellNode));

			this.nvCells.AppendColumn ("Name", new Gtk.CellRendererText (), "text", 0);
			this.nvCells.NodeStore = this.CellStore;

			Pango.FontDescription font = new Pango.FontDescription ();
			font.Family = "Dotum";
			font.Size = 12;
			font.Weight = Pango.Weight.Bold;
			this.lblName.ModifyFont (font);
		}

		private void UpdateCellList()
		{
			this.CellStore.Clear ();

			foreach (var CellPath in Directory.EnumerateFiles (this.assetPath + System.IO.Path.DirectorySeparatorChar + "cells", "*.cell"))
			{
				this.CellStore.AddNode (new CellNode (CellPath));
			}
		}

		private void UpdateCell()
		{
		}

		protected void OnBtnNewCellClicked (object sender, EventArgs e)
		{
			using (InputTextDialog dlg = new InputTextDialog ()) {
				dlg.Message = "Enter new cell name";
				dlg.Text = "unnamed";
				if (dlg.Run () == (int)ResponseType.Ok) {
					string CellName = dlg.Text;
					string CellPath = this.AssetPath + System.IO.Path.DirectorySeparatorChar + "cells" + System.IO.Path.DirectorySeparatorChar + CellName + ".cell";
					if (File.Exists (CellPath)) {
						using (MessageDialog dlg2 = new MessageDialog (null, DialogFlags.Modal, MessageType.Warning, ButtonsType.YesNo, "Cell by that name already exists!\n\nOverwrite?")) {
							if (dlg2.Run () != (int)ResponseType.Yes) {
								return;
							}
						}
					}

					this.Cell = new CellInfo (CellName, CellPath);
					this.Cell.Save ();
				}
			}
		}
	}
}

