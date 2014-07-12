#ifndef BARFOOS_MOB_H
#define BARFOOS_MOB_H

#include "entity.h"

class Mob : public Entity {
public:

                            Mob         (const std::string &type);
                            Mob         (const Entity_Proto &proto);
                            Mob         (const Mob &that) = delete;
  virtual                   ~Mob        ();

  Mob &                     operator=   (const Mob &that) = delete;

  virtual void              Start       (RunningState &state, ID id)                  override;
  virtual void              Continue    (RunningState &state, ID id)                  override;
  virtual void              Update      (RunningState &state)                         override;
  virtual void              Think       (RunningState &state)                         override;

  virtual void              OnCollide   (RunningState &state, Entity &other)          override;

  void                      ApplyForce  (RunningState &state, const Vector3 &f);
  void                      AddImpulse  (const Vector3 &v)                                            { this->AddVelocity(v / this->properties->mass); }
  void                      AddVelocity (const Vector3 &v)                                            { this->SetVelocity(this->GetVelocity() + v); }
  void                      SetVelocity (const Vector3 &v)                                            { this->proto.mutable_mob()->mutable_velocity()->set_x(v.x);
                                                                                                        this->proto.mutable_mob()->mutable_velocity()->set_y(v.y);
                                                                                                        this->proto.mutable_mob()->mutable_velocity()->set_z(v.z); }
  Vector3                   GetVelocity ()                                            const           { return Vector3( this->proto.mob().velocity().x(),
                                                                                                                        this->proto.mob().velocity().y(),
                                                                                                                        this->proto.mob().velocity().z() ); }

  void                      SetOnGround (bool v)                                                      { this->proto.mutable_mob()->set_is_on_ground(v); }
  void                      SetNoClip   (bool v)                                                      { this->proto.mutable_mob()->set_is_noclip(v); }
  void                      SetSneaking (bool v)                                                      { this->proto.mutable_mob()->set_is_sneaking(v); }
  void                      SetInLiquid (bool v);
  void                      SetSubmerged(bool v)                                                      { this->proto.mutable_mob()->set_is_submerged(v); }

  bool                      IsOnGround  ()                                            const           { return this->proto.mob().is_on_ground(); }
  bool                      IsNoclip    ()                                            const           { return this->proto.mob().is_noclip(); }
  bool                      IsSneaking  ()                                            const           { return this->proto.mob().is_sneaking(); }
  bool                      IsInLiquid  ()                                            const           { return this->proto.mob().is_in_liquid(); }
  bool                      IsSubmerged ()                                            const           { return this->proto.mob().is_submerged(); }

  Cell *                    GetSelection    (RunningState &state, float range, const std::shared_ptr<Item> &item, Side &selectedCellSide, ID &entityId);
  bool                      HasLearntSpell  (const std::string &name) const;
  virtual void              LearnSpell      (const std::string &name);

protected:

  Vector3               move;
  bool                  doesWantJump;

  Cell *                headCell;
  Cell *                footCell;
  Cell *                groundCell;

  float                     GetLastJumpTime () const  { return this->proto.mob().last_jump_time(); }
  void                      SetLastJumpTime (float t) { this->proto.mutable_mob()->set_last_jump_time(t); }

  float                     GetMoveModifier () const;
};



#endif

