#ifndef BARFOOS_WORLDBUILDER_H
#define BARFOOS_WORLDBUILDER_H

#include <vector>

#include "ivector3.h"

struct Theme {
  size_t featureCount       = 200;
  size_t minFeatures        = 100;

  float  useLastChance      = 0.5;
  float  useLastDirChance   = 0.6;
  size_t caveLengthMin      = 20;
  size_t caveLengthMax      = 100;
  size_t caveRepeat         = 10;

  size_t teleportCount      = 10;
  size_t trapCount          = 10;
  size_t decoCount          = 500;
  size_t itemCount          = 100;
  size_t monsterCount       = 50;

  std::string firstFeature  = "start";
  IVector3 firstFeaturePos  = IVector3(32-4, 32-8,32-4);
};

class WorldBuilder final {
public:

  WorldBuilder(World &world);
  ~WorldBuilder();

  void Build(RunningState &state, const Theme &theme);

private:

  World &world;
  std::vector<FeatureInstance> instances;
  std::vector<Cell> defaultCells;
  uint32_t minY, maxY;
  int lastDir;
  size_t loop;

  void MakeGround(Random &random);
  void FillGround();
  void BuildFeature(RunningState &state, Random &random, const Theme &theme);
  void MakeCaves(Random &random, const Theme &theme);
  void PlaceTeleports(Random &random, const Theme &theme);
  void PlaceTraps(RunningState &state, Random &random, const Theme &theme);
};

#endif

