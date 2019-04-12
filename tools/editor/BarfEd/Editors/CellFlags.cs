using System;

namespace Editors
{
	public class CellFlags
	{
		public const int Solid = 1;
		public const int Transparent = 2;
		public const int Dynamic = 4;
		public const int DoNotRender = 8;
		public const int Liquid = 16;
		public const int Viscous = 32;
		public const int MultiSided = 64;
		public const int UVTurb = 128;
		public const int Waving = 256;
		public const int DoubleSided = 512;
		public const int Pickable = 1024;
		public const int OnUseReplace = 2048;
		public const int Ladder = 4096;
	}
}

