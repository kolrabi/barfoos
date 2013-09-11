#ifndef BARFOOS_MONSTER_H
#define BARFOOS_MONSTER_H

#include "mob.h"

class Monster : public Mob {
public:

                            Monster(const std::string &type);
                            Monster(const Entity_Proto &proto);
                            Monster(const Monster &that) = delete;

  virtual                   ~Monster();

  Monster &                 operator=   (const Monster &that) = delete;

  virtual void              Start       (RunningState &state, ID id)                  override;
  virtual void              Continue    (RunningState &state, ID id)                  override;
  virtual void              Update      (RunningState &state)                         override;
  virtual void              Think       (RunningState &state)                         override;

  virtual void              OnCollide   (RunningState &state, Entity &other)          override;
  virtual void              AddHealth   (RunningState &state, const HealthInfo &info)    override;

protected:

  float                     GetNextAttackTime()                                           const { return this->proto.monster().next_attack_time(); }
  void                      SetNextAttackTime(float t)                                          { this->proto.mutable_monster()->set_next_attack_time(t); }

  float                     GetNextMoveTime()                                             const { return this->proto.monster().next_move_time(); }
  void                      SetNextMoveTime(float   t)                                          { this->proto.mutable_monster()->set_next_move_time(t); }

  bool                      IsMoveTargetValid()                                           const { return this->proto.monster().has_move_target(); }
  void                      ClearMoveTarget()                                                   { this->proto.mutable_monster()->clear_move_target(); }
  void                      SetMoveTarget(const Vector3 &target)                                { this->proto.mutable_monster()->mutable_move_target()->set_x(target.x);
                                                                                                  this->proto.mutable_monster()->mutable_move_target()->set_y(target.y);
                                                                                                  this->proto.mutable_monster()->mutable_move_target()->set_z(target.z); }
  Vector3                   GetMoveTarget()                                               const { return Vector3( this->proto.monster().move_target().x(),
                                                                                                                  this->proto.monster().move_target().y(),
                                                                                                                  this->proto.monster().move_target().z() ); }

  bool                      IsAttackTargetValid()                                         const { return this->proto.monster().has_attack_target(); }
  void                      ClearAttackTarget()                                                 { this->proto.mutable_monster()->clear_attack_target(); }
  void                      SetAttackTarget(ID target)                                          { this->proto.mutable_monster()->set_attack_target(target); }
  ID                        GetAttackTarget()                                             const { return this->proto.monster().attack_target(); }


  // from properties:
  std::shared_ptr<Item>     attackItem;
};



#endif

