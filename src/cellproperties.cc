#include "cellproperties.h"

#include "util.h"
#include "gfx.h"

#include "texture.h"
 
#include <unordered_map>
#include <cstdio>

static std::unordered_map<std::string, CellProperties> cellProperties;

CellProperties::CellProperties() :
  type("default"),
  textures(0),
  emissiveTextures(0),
  activeTexture(""),
  emissiveActiveTexture(""),
  light(0,0,0),
  flags(0),
  lightFactor(0.85),
  lightFade(0),
  replace(""),
  replaceChance(0.0),
  detailBelowReplace(0),
  scale(1.0, 1.0, 1.0),
  speedModifier(1.0),
  friction(1.0),
  showSides(0),
  hideSides(0),
  clipSidesIn(0),
  clipSidesOut(0),
  onUseCascade(0),
  useDelay(0.0),
  breakStrength(-1.0),
  lavaDamage(0.0),
  useChance(1.0),
  breakParticle("particle.rock"),
  lockedChance(0.0)
{}

void CellProperties::ParseProperty(const std::string &cmd) {
  if (cmd == "tex")               Parse("cells/texture/", this->textures);
  else if (cmd == "emissivetex")  Parse("cells/texture/", this->emissiveTextures);
  else if (cmd == "activetex")    Parse(this->activeTexture);
  else if (cmd == "emissiveactivetex")    Parse(this->emissiveActiveTexture);
  
  else if (cmd == "light")        Parse(this->light);
  else if (cmd == "lightscale")   { float f; Parse(f); this->light = this->light * f; }
  else if (cmd == "lightfactor")  Parse(this->lightFactor);
  else if (cmd == "lightfade")    Parse(this->lightFade);
  
  else if (cmd == "uvturb")       this->flags |= UVTurb | Dynamic;
  else if (cmd == "wave")         this->flags |= Waving | Dynamic;
  else if (cmd == "solid")      { this->flags |= Solid  | Pickable; this->clipSidesIn = ~0; }
  else if (cmd == "dynamic")      this->flags |= Dynamic;
  else if (cmd == "liquid")       this->flags |= Liquid;
  else if (cmd == "norender")     this->flags |= DoNotRender;
  else if (cmd == "transparent")  this->flags |= Transparent;
  else if (cmd == "doublesided")  this->flags |= DoubleSided;
  else if (cmd == "pickable")     this->flags |= Pickable; 
  else if (cmd == "onusereplace") this->flags |= OnUseReplace;
  else if (cmd == "multi")        this->flags |= MultiSided;
  else if (cmd == "ladder")       this->flags |= Ladder;
  
  else if (cmd == "speed")        Parse(this->speedModifier);
  else if (cmd == "friction")     Parse(this->friction);
  
  else if (cmd == "showsides")    ParseSideMask(this->showSides);
  else if (cmd == "hidesides")    ParseSideMask(this->hideSides);
  else if (cmd == "clipsidesin")  ParseSideMask(this->clipSidesIn);
  else if (cmd == "clipsidesout") ParseSideMask(this->clipSidesOut);
  
  else if (cmd == "onusecascade") ParseSideMask(this->onUseCascade);
  else if (cmd == "usedelay")     Parse(this->useDelay);
  else if (cmd == "usechance")    Parse(this->useChance); 
  
  else if (cmd == "replace")      Parse(this->replace);

  else if (cmd == "strength")     Parse(this->breakStrength);
  else if (cmd == "lavadamage")   Parse(this->lavaDamage);
  
  else if (cmd == "scale")        Parse(this->scale);
  else if (cmd == "breakparticle")     Parse(this->breakParticle);
  
  else if (cmd == "detailbelowreplace") Parse(this->detailBelowReplace);
  else if (cmd == "replacechance") Parse(this->replaceChance); 
  else if (cmd == "onflowontoreplacetarget") {
    std::string from, to, own;
    Parse(from);
    Parse(to);
    Parse(own);
    this->onFlowOntoReplaceTarget[from] = to;
    this->onFlowOntoReplaceSelf[from] = own;
  }
  else if (cmd == "lockedchance") Parse(this->lockedChance);
  else SetError("Ignoring '" + cmd + "'\n");
}

void LoadCells() {
  std::vector<std::string> assets = findAssets("cells");
  for (const std::string &name : assets) {
    FILE *f = openAsset("cells/"+name);
    if (f) {
      Log("Loading cell properties for type '%s'\n", name.c_str());
      cellProperties[name].ParseFile(f);
      cellProperties[name].type = name;
      fclose(f);
    }
  }
}

const CellProperties &GetCellProperties(const std::string &type) {
  return cellProperties[type];
}

