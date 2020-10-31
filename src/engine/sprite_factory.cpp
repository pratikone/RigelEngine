/* Copyright (C) 2020, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sprite_factory.hpp"

#include "base/container_utils.hpp"
#include "data/unit_conversions.hpp"
#include "loader/actor_image_package.hpp"


namespace rigel::engine {

using data::ActorID;
using namespace engine::components;

namespace {

auto createFrameDrawData(
  const loader::ActorData::Frame& frameData,
  renderer::Renderer* pRenderer
) {
  auto texture = renderer::OwningTexture{pRenderer, frameData.mFrameImage};
  return engine::SpriteFrame{std::move(texture), frameData.mDrawOffset};
}


void applyTweaks(
  std::vector<engine::SpriteFrame>& frames,
  const ActorID actorId,
  const std::vector<loader::ActorData>& actorParts,
  renderer::Renderer* pRenderer
) {
  // Some sprites in the game have offsets that would require more complicated
  // code to draw them correctly. To simplify that, we adjust the offsets once
  // at loading time so that no additional adjustment is necessary at run time.

  // Player sprite
  if (actorId == ActorID::Duke_LEFT || actorId == ActorID::Duke_RIGHT) {
    for (int i=0; i<39; ++i) {
      if (i != 35 && i != 36) {
        frames[i].mDrawOffset.x -= 1;
      }
    }
  }

  // Destroyed reactor fire
  if (actorId == ActorID::Reactor_fire_LEFT || actorId == ActorID::Reactor_fire_RIGHT) {
    frames[0].mDrawOffset.x = 0;
  }

  // Radar computer
  if (actorId == ActorID::Radar_computer_terminal) {
    for (auto i = 8u; i < frames.size(); ++i) {
      frames[i].mDrawOffset.x -= 1;
    }
  }

  // Duke's ship
  if (
    actorId == ActorID::Dukes_ship_LEFT ||
    actorId == ActorID::Dukes_ship_RIGHT ||
    actorId == ActorID::Dukes_ship_after_exiting_LEFT ||
    actorId == ActorID::Dukes_ship_after_exiting_RIGHT
  ) {
    // The incoming frame list is based on IDs 87, 88, and 92. The frames
    // are laid out as follows:
    //
    //  0, 1: Duke's ship, facing right
    //  2, 3: Duke's ship, facing left
    //  4, 5: exhaust flames, facing down
    //  6, 7: exhaust flames, facing left
    //  8, 9: exhaust flames, facing right
    //
    // In order to display the down facing exhaust flames correctly when
    // Duke's ship is facing left, we need to apply an additional X offset to
    // frames 4 and 5. But currently, RigelEngine doesn't support changing the
    // X offset temporarily, so we need to first create a copy of those frames,
    // insert them after 8 and 9, and then adjust their offset.
    //
    // After this tweak, the frame layout is as follows:
    //
    //   0,  1: Duke's ship, facing right
    //   2,  3: Duke's ship, facing left
    //   4,  5: exhaust flames, facing down, x-offset for facing left
    //   6,  7: exhaust flames, facing left
    //   8,  9: exhaust flames, facing down, x-offset for facing right
    //  10, 11: exhaust flames, facing right
    frames.insert(
      frames.begin() + 8,
      createFrameDrawData(actorParts[2].mFrames[0], pRenderer));
    frames.insert(
      frames.begin() + 9,
      createFrameDrawData(actorParts[2].mFrames[1], pRenderer));

    frames[8].mDrawOffset.x += 1;
    frames[9].mDrawOffset.x += 1;
  }

  if (actorId == ActorID::Bomb_dropping_spaceship) {
    frames[3].mDrawOffset += base::Vector{2, 0};
    frames.erase(std::next(frames.begin(), 4), frames.end());
  }

  if (actorId == ActorID::Watchbot_container_carrier) {
    frames[2].mDrawOffset += base::Vector{0, -2};
    frames.erase(std::next(frames.begin(), 3), frames.end());
  }
}


std::optional<int> orientationOffsetForActor(const ActorID actorId) {
  switch (actorId) {
    case ActorID::Duke_LEFT:
    case ActorID::Duke_RIGHT:
      return 39;

    case ActorID::Snake:
      return 9;

    case ActorID::Eyeball_thrower_LEFT:
      return 10;

    case ActorID::Skeleton:
      return 4;

    case ActorID::Spider:
      return 13;

    case ActorID::Red_box_turkey:
      return 2;

    case ActorID::Rigelatin_soldier:
      return 4;

    case ActorID::Ugly_green_bird:
      return 3;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
      return 3;

    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
      return 6;

    case ActorID::Unicycle_bot:
      return 4;

    case ActorID::Dukes_ship_LEFT:
    case ActorID::Dukes_ship_RIGHT:
    case ActorID::Dukes_ship_after_exiting_LEFT:
    case ActorID::Dukes_ship_after_exiting_RIGHT:
      return 6;

    default:
      return std::nullopt;
  }
}


int SPIDER_FRAME_MAP[] = {
  3, 4, 5, 9, 10, 11, 6, 8, 9, 14, 15, 12, 13, // left
  0, 1, 2, 6, 7, 8, 6, 8, 9, 12, 13, 14, 15, // right
};


int UNICYCLE_FRAME_MAP[] = {
  0, 5, 1, 2, // left
  0, 5, 3, 4, // right
};


int DUKES_SHIP_FRAME_MAP[] = {
  0, 1, 10, 11, 8, 9, // left
  2, 3, 6, 7, 4, 5, // right
};


base::ArrayView<int> frameMapForActor(const ActorID actorId) {
  switch (actorId) {
    case ActorID::Spider:
      return base::ArrayView<int>(SPIDER_FRAME_MAP);

    case ActorID::Unicycle_bot:
      return base::ArrayView<int>(UNICYCLE_FRAME_MAP);

    case ActorID::Dukes_ship_LEFT:
    case ActorID::Dukes_ship_RIGHT:
    case ActorID::Dukes_ship_after_exiting_LEFT:
    case ActorID::Dukes_ship_after_exiting_RIGHT:
      return base::ArrayView<int>(DUKES_SHIP_FRAME_MAP);

    default:
      return {};
  }
}


auto actorIDListForActor(const ActorID ID) {
  std::vector<ActorID> actorParts;

  switch (ID) {
    case ActorID::Hoverbot:
      actorParts.push_back(ActorID::Hoverbot);
      actorParts.push_back(ActorID::Hoverbot_teleport_FX);
      break;

    case ActorID::Duke_LEFT:
    case ActorID::Duke_RIGHT:
      actorParts.push_back(ActorID::Duke_LEFT);
      actorParts.push_back(ActorID::Duke_RIGHT);
      break;

    case ActorID::Blue_bonus_globe_1:
    case ActorID::Blue_bonus_globe_2:
    case ActorID::Blue_bonus_globe_3:
    case ActorID::Blue_bonus_globe_4:
      actorParts.push_back(ID);
      actorParts.push_back(ActorID::Bonus_globe_shell);
      break;

    case ActorID::Teleporter_1:
      actorParts.push_back(ActorID::Teleporter_2);
      break;

    case ActorID::Green_slime_blob:
      actorParts.push_back(ActorID::Green_slime_blob);
      actorParts.push_back(ActorID::Green_slime_blob_flying_on_ceiling);
      break;

    case ActorID::Eyeball_thrower_LEFT:
      actorParts.push_back(ActorID::Eyeball_thrower_LEFT);
      actorParts.push_back(ActorID::Eyeball_thrower_RIGHT);
      break;

    case ActorID::Bomb_dropping_spaceship:
      actorParts.push_back(ActorID::Bomb_dropping_spaceship);
      actorParts.push_back(ActorID::Napalm_bomb);
      break;

    case ActorID::Blowing_fan:
      actorParts.push_back(ActorID::Blowing_fan);
      actorParts.push_back(ActorID::Blowing_fan_threads_on_top);
      break;

    case ActorID::Missile_intact:
      actorParts.push_back(ActorID::Missile_intact);
      actorParts.push_back(ActorID::Missile_exhaust_flame);
      break;

    case ActorID::Blue_guard_LEFT:
    case ActorID::Blue_guard_using_a_terminal:
      actorParts.push_back(ActorID::Blue_guard_RIGHT);
      break;

    case ActorID::Enemy_laser_shot_LEFT:
    case ActorID::Enemy_laser_shot_RIGHT:
      actorParts.push_back(ActorID::Enemy_laser_shot_RIGHT);
      break;

    case ActorID::Red_box_turkey:
      actorParts.push_back(ActorID::Turkey);
      break;

    case ActorID::Messenger_drone_1:
    case ActorID::Messenger_drone_2:
    case ActorID::Messenger_drone_3:
    case ActorID::Messenger_drone_4:
    case ActorID::Messenger_drone_5:
      actorParts.push_back(ActorID::Messenger_drone_body);
      actorParts.push_back(ActorID::Messenger_drone_part_1);
      actorParts.push_back(ActorID::Messenger_drone_part_2);
      actorParts.push_back(ActorID::Messenger_drone_part_3);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_1);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_2);
      actorParts.push_back(ActorID::Messenger_drone_exhaust_flame_3);
      actorParts.push_back(ID);
      break;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
      actorParts.push_back(ActorID::Big_green_cat_LEFT);
      actorParts.push_back(ActorID::Big_green_cat_RIGHT);
      break;

    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
      actorParts.push_back(ActorID::Spiked_green_creature_LEFT);
      actorParts.push_back(ActorID::Spiked_green_creature_RIGHT);
      break;

    case ActorID::Dukes_ship_LEFT:
    case ActorID::Dukes_ship_RIGHT:
    case ActorID::Dukes_ship_after_exiting_LEFT:
    case ActorID::Dukes_ship_after_exiting_RIGHT:
      actorParts.push_back(ActorID::Dukes_ship_LEFT);
      actorParts.push_back(ActorID::Dukes_ship_RIGHT);
      actorParts.push_back(ActorID::Dukes_ship_exhaust_flames);
      break;

    case ActorID::Watchbot_container_carrier:
      actorParts.push_back(ActorID::Watchbot_container_carrier);
      actorParts.push_back(ActorID::Watchbot_container);
      break;

    default:
      actorParts.push_back(ID);
      break;
  }
  return actorParts;
}


void configureSprite(Sprite& sprite, const ActorID actorID) {
  switch (actorID) {
    case ActorID::Hoverbot:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Bomb_dropping_spaceship:
      sprite.mFramesToRender = {3, 0, 1};
      break;

    case ActorID::Green_slime_blob:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Eyeball_thrower_LEFT:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Sentry_robot_generator:
      sprite.mFramesToRender = {0, 4};
      break;

    case ActorID::Missile_intact:
      sprite.mFramesToRender = {0};
      break;

    case ActorID::Metal_grabber_claw:
      sprite.mFramesToRender = {1};
      break;

    case ActorID::Spider:
      sprite.mFramesToRender = {6};
      break;

    case ActorID::Blue_guard_LEFT:
      sprite.mFramesToRender = {6};
      break;

    case ActorID::BOSS_Episode_1:
      sprite.mFramesToRender = {0, 2};
      break;

    case ActorID::BOSS_Episode_3:
      sprite.mFramesToRender = {engine::IGNORE_RENDER_SLOT, 1, 0};
      break;

    case ActorID::BOSS_Episode_4:
      sprite.mFramesToRender = {0, 1};
      break;

    case ActorID::Rocket_elevator:
      sprite.mFramesToRender = {5, 0};
      break;

    case ActorID::Blue_guard_using_a_terminal:
      sprite.mFramesToRender = {12};
      break;

    case ActorID::Lava_fountain:
      // Handled by custom render func
      sprite.mFramesToRender = {};
      break;

    case ActorID::Radar_computer_terminal:
      sprite.mFramesToRender = {0, 1, 2, 3};
      break;

    case ActorID::Watchbot_container:
      sprite.mFramesToRender = {0, 1};
      break;

    case ActorID::Watchbot_container_carrier:
      sprite.mFramesToRender = {0, 2};
      break;

    case ActorID::Super_force_field_LEFT:
      sprite.mFramesToRender = {0, 3};
      break;

    case ActorID::Big_green_cat_LEFT:
    case ActorID::Big_green_cat_RIGHT:
    case ActorID::Spiked_green_creature_LEFT:
    case ActorID::Spiked_green_creature_RIGHT:
    case ActorID::Duke_LEFT:
    case ActorID::Duke_RIGHT:
    case ActorID::Dukes_ship_LEFT:
    case ActorID::Dukes_ship_RIGHT:
    case ActorID::Dukes_ship_after_exiting_LEFT:
    case ActorID::Dukes_ship_after_exiting_RIGHT:
      sprite.mFramesToRender = {0};
      break;

    default:
      break;
  }
}


int adjustedDrawOrder(const ActorID id, const int baseDrawOrder) {
  auto scale = [](const int drawOrderValue) {
    constexpr auto SCALE_FACTOR = 10;
    return drawOrderValue * SCALE_FACTOR;
  };

  switch (id) {
    case ActorID::Duke_rocket_up: case ActorID::Duke_rocket_down: case ActorID::Duke_rocket_left: case ActorID::Duke_rocket_right:
    case ActorID::Duke_laser_shot_horizontal: case ActorID::Duke_laser_shot_vertical: case ActorID::Duke_regular_shot_horizontal: case ActorID::Duke_regular_shot_vertical:
    case ActorID::Duke_flame_shot_up: case ActorID::Duke_flame_shot_down: case ActorID::Duke_flame_shot_left: case ActorID::Duke_flame_shot_right:
    case ActorID::Reactor_fire_LEFT: case ActorID::Reactor_fire_RIGHT:
      return scale(PLAYER_PROJECTILE_DRAW_ORDER);

    case ActorID::Muzzle_flash_up: case ActorID::Muzzle_flash_down: case ActorID::Muzzle_flash_left: case ActorID::Muzzle_flash_right: // player muzzle flash
      return scale(MUZZLE_FLASH_DRAW_ORDER);

    case ActorID::Explosion_FX_1:
    case ActorID::Explosion_FX_2:
    case ActorID::Shot_impact_FX:
    case ActorID::Smoke_puff_FX:
    case ActorID::Hoverbot_debris_1:
    case ActorID::Hoverbot_debris_2:
    case ActorID::Nuclear_waste_can_debris_1:
    case ActorID::Nuclear_waste_can_debris_2:
    case ActorID::Nuclear_waste_can_debris_3:
    case ActorID::Nuclear_waste_can_debris_4:
    case ActorID::Flame_thrower_fire_RIGHT:
    case ActorID::Flame_thrower_fire_LEFT:
    case ActorID::Nuclear_explosion:
    case ActorID::Watchbot_container_debris_1:
    case ActorID::Watchbot_container_debris_2:
    case ActorID::Fire_bomb_fire:
    case ActorID::Duke_death_particles:
    case ActorID::Bonus_globe_debris_1:
    case ActorID::Bonus_globe_debris_2:
    case ActorID::White_circle_flash_FX:
    case ActorID::Nuclear_waste_can_green_slime_inside:
    case ActorID::Smoke_cloud_FX:
    case ActorID::Biological_enemy_debris:
    case ActorID::Missile_debris:
    case ActorID::Eyeball_projectile:
    case ActorID::Enemy_laser_muzzle_flash_1:
    case ActorID::Enemy_laser_muzzle_flash_2:
    case ActorID::Metal_grabber_claw_debris_1:
    case ActorID::Metal_grabber_claw_debris_2:
    case ActorID::Yellow_fireball_FX:
    case ActorID::Green_fireball_FX:
    case ActorID::Blue_fireball_FX:
    case ActorID::Coke_can_debris_1:
    case ActorID::Coke_can_debris_2:
    case ActorID::Spiked_green_creature_eye_FX_LEFT:
    case ActorID::Spiked_green_creature_eye_FX_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_1_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_2_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_3_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_4_LEFT:
    case ActorID::Spiked_green_creature_stone_debris_1_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_2_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_3_RIGHT:
    case ActorID::Spiked_green_creature_stone_debris_4_RIGHT:
    case ActorID::Spider_shaken_off:
    case ActorID::Windblown_spider_generator:
    case ActorID::Spider_debris_2:
    case ActorID::Spider_blowing_in_wind:
    case ActorID::Prisoner_hand_debris:
    case ActorID::Rigelatin_soldier_projectile:
      return scale(EFFECT_DRAW_ORDER);

    case ActorID::Score_number_FX_100:
    case ActorID::Score_number_FX_500:
    case ActorID::Score_number_FX_2000:
    case ActorID::Score_number_FX_5000:
    case ActorID::Score_number_FX_10000:
      return scale(EFFECT_DRAW_ORDER);

    case ActorID::Napalm_bomb:
      // Make the bomb appear behind the bomber plane
      return scale(baseDrawOrder) - 1;

    default:
      return scale(baseDrawOrder);
  }
}

}


SpriteFactory::SpriteFactory(
  renderer::Renderer* pRenderer,
  const loader::ActorImagePackage* pSpritePackage
)
  : mpRenderer(pRenderer)
  , mpSpritePackage(pSpritePackage)
{
}


Sprite SpriteFactory::createSprite(const ActorID id) {
  const auto& data = createOrFindData(id);
  auto sprite = Sprite{&data.mDrawData, data.mInitialFramesToRender};
  configureSprite(sprite, id);
  return sprite;
}


base::Rect<int> SpriteFactory::actorFrameRect(
  const data::ActorID id,
  const int frame
) const {
  const auto& data = createOrFindData(id);
  const auto realFrame = virtualToRealFrame(0, data.mDrawData, std::nullopt);
  const auto& frameData = data.mDrawData.mFrames[realFrame];

  const auto dimensionsInTiles = data::pixelExtentsToTileExtents(
    frameData.mImage.extents());
  return {frameData.mDrawOffset, dimensionsInTiles};
}

auto SpriteFactory::createOrFindData(const ActorID mainId) const
  -> const SpriteData&
{
  auto iData = mSpriteDataCache.find(mainId);
  if (iData == mSpriteDataCache.end()) {
    engine::SpriteDrawData drawData;

    int lastDrawOrder = 0;
    int lastFrameCount = 0;
    std::vector<int> framesToRender;

    const auto actorPartIds = actorIDListForActor(mainId);
    const auto actorParts = utils::transformed(actorPartIds,
      [&](const ActorID partId) {
        return mpSpritePackage->loadActor(partId);
      });

    for (const auto& actorData : actorParts) {
      lastDrawOrder = actorData.mDrawIndex;

      for (const auto& frameData : actorData.mFrames) {
        drawData.mFrames.emplace_back(
          createFrameDrawData(frameData, mpRenderer));
      }

      framesToRender.push_back(lastFrameCount);
      lastFrameCount = int(actorData.mFrames.size());
    }

    drawData.mOrientationOffset = orientationOffsetForActor(mainId);
    drawData.mVirtualToRealFrameMap = frameMapForActor(mainId);
    drawData.mDrawOrder = adjustedDrawOrder(mainId, lastDrawOrder);

    applyTweaks(drawData.mFrames, mainId, actorParts, mpRenderer);

    iData = mSpriteDataCache.emplace(
      mainId,
      SpriteData{std::move(drawData), std::move(framesToRender)}
    ).first;
  }

  return iData->second;
}

}
