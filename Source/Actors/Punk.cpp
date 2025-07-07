//
// Created by Lucas N. Ferreira on 03/08/23.
//

#include "Punk.h"
#include "Block.h"
#include "Projectile.h"
#include "ProjectileEffect.h"
#include "../Game.h"
#include "Portal.h"
#include "../Components/DrawComponents/DrawSpriteComponent.h"
#include "../Components/DrawComponents/DrawAnimatedComponent.h"
#include "../Components/DrawComponents/DrawPolygonComponent.h"
#include "../UIElements/DialogueSystem.h"

Punk::Punk(Game *game, const float forwardSpeed, const float jumpSpeed)
    : Actor(game), mIsRunning(false), mIsOnPole(false), mIsDying(false),
      mForwardSpeed(forwardSpeed), mJumpSpeed(jumpSpeed), mPoleSlideTimer(0.0f),
      mFoundKey(false), mDeathTimer(0.0f), mCallBackCooldown(3.0f), mCallBackTimer(0.0f)
{
    mRigidBodyComponent = new RigidBodyComponent(this, 1.0f, 5.0f, false);
    mColliderComponent = new AABBColliderComponent(this, 14, 20, 18, 28,
                                                   ColliderLayer::Player);

    mDrawComponent = new DrawAnimatedComponent(this,
                                               "../Assets/Sprites/Punk/texture.png",
                                               "../Assets/Sprites/Punk/texture.json",
                                               static_cast<int>(DrawLayerPosition::Player) + 1);

    mDrawComponent->AddAnimation("idle", {0, 1, 2, 3});
    mDrawComponent->AddAnimation("run", {4, 5, 6, 7, 8, 9, 10});
    mDrawComponent->AddAnimation("dying", {11, 12, 13, 14, 15, 16});
    mDrawComponent->AddAnimation("shooting_left_arm", {3});
    mDrawComponent->AddAnimation("shooting_noarm", {17});
    mDrawComponent->AddAnimation("dash", {18, 19, 20, 21, 22, 23});

    mDrawComponent->SetAnimation("idle");
    mDrawComponent->SetAnimFPS(13.0f);

    mArm = new PunkArm(mGame, this, [this](Vector2 &recoilDir)
                       { OnShoot(recoilDir); });

    mShortcutCallbacks = {
        [this]()
        {SetPosition(Vector2(608.0f, 832.0f));},
        [this]()
        {SetPosition(Vector2(1490.0f, 100.0f));},
        [this]()
        {SetPosition(Vector2(622.0f, 150.0f));}
    };
}

void Punk::OnShoot(Vector2 &recoilForce)
{
    mRigidBodyComponent->ApplyForce(recoilForce);
}

void Punk::OnProcessInput(const uint8_t *state)
{
    // if(mGame->GetGamePlayState() != Game::GamePlayState::Playing) return;
    if (mIsDying)
        return;

    mIsRunning = false;

    if (mArm->mIsShooting)
        return;

    if (state[SDL_SCANCODE_D])
    {
        mRigidBodyComponent->ApplyForce(Vector2(mForwardSpeed, 0.0f));
        SetRotation(0.0f);
        mIsRunning = true;
    }
    if (state[SDL_SCANCODE_A])
    {
        mRigidBodyComponent->ApplyForce(Vector2(-mForwardSpeed, 0.0f));
        SetRotation(Math::Pi);
        mIsRunning = true;
    }

    if (state[SDL_SCANCODE_W])
    {
        mRigidBodyComponent->ApplyForce(Vector2(0.0f, -mForwardSpeed));
        mIsRunning = true;
    }

    if (state[SDL_SCANCODE_S])
    {
        mRigidBodyComponent->ApplyForce(Vector2(0.0f, mForwardSpeed));
        mIsRunning = true;
    }

    if (state[SDL_SCANCODE_F])
    {
        if (mArm->mFoundShotgun == false)
            return;
        mArm->ChangeWeapon();
    }

    if (state[SDL_SCANCODE_G] && mCallBackTimer <= 0.0f)
    {
        mShortcutCallbacks[0]();
        mShortcutCallbacks.erase(mShortcutCallbacks.begin());
        mCallBackTimer = mCallBackCooldown;
    }
}

void Punk::OnHandleKeyPress(const int key, const bool isPressed)
{
}

void Punk::TakeDamage()
{
    if (mIsDying)
        return;

    if (mInvincibilityTimer > 0.0f)
        return;

    mLives--;
    mInvincibilityTimer = 0.25f;

    if (mLives <= 0)
    {
        mIsDying = true;
        mDrawComponent->SetAnimFPS(8.0f);
        mDeathTimer = DEATH_TIMER;
    }
}

void Punk::MaintainInbound()
{
    Vector2 cameraPos = GetGame()->GetCameraPos();
    Vector2 getUpperLeftBorder = mColliderComponent->GetMin();
    Vector2 getBottomRightBorder = mColliderComponent->GetMax();
    Vector2 offset = mColliderComponent->GetOffset();
    int mWidth = mColliderComponent->GetWidth();
    int mHeight = mColliderComponent->GetHeight();
    int maxXBoundary = cameraPos.x + GetGame()->GetWindowWidth();
    int maxYBoundary = cameraPos.y + GetGame()->GetWindowHeight();

    if (getUpperLeftBorder.x < 0)
    {
        SetPosition(Vector2(-offset.x, GetPosition().y));
    }
    else if (getBottomRightBorder.x > maxXBoundary)
    {
        SetPosition(Vector2(maxXBoundary - mWidth - offset.x, GetPosition().y));
    }

    if (getUpperLeftBorder.y < 0)
    {
        SetPosition(Vector2(GetPosition().x, -offset.y));
    }
    else if (getBottomRightBorder.y > maxYBoundary)
    {
        SetPosition(Vector2(GetPosition().x, maxYBoundary - mHeight - offset.y));
    }
}

void Punk::OnUpdate(float deltaTime)
{
    if (mCallBackTimer > 0.0f) mCallBackTimer -= deltaTime;
    else mCallBackTimer = 0.0f;
    
    if (mGame->GetGamePlayState() == Game::GamePlayState::Dialogue)
    {
        mDrawComponent->SetAnimation("idle");
        return;
    }

    mArm->SetPosition(GetPosition());

    MaintainInbound();
    ManageAnimations();

    if (mArm->IsAimingRight())
        SetRotation(0.0f);
    else if (mArm->IsAimingLeft())
        SetRotation(Math::Pi);

    if (mIsDying)
    {
        mDeathTimer -= deltaTime;

        if (mDeathTimer <= 0)
        {
            mGame->Quit();
        }
        return;
    }

    if (mInvincibilityTimer > 0.0f)
        mInvincibilityTimer -= deltaTime;
}

void Punk::ManageAnimations()
{
    if (mIsDying)
    {
        mDrawComponent->SetAnimation("dying");
    }
    else if (mArm->mIsShooting)
    {
        mDrawComponent->SetAnimation(mArm->ArmConfig());
    }
    else if (mIsRunning)
    {
        mDrawComponent->SetAnimation("run");
    }
    else if (!mIsRunning)
    {
        mDrawComponent->SetAnimation("idle");
    }
}

void Punk::Kill()
{
    mIsDying = true;
    mGame->SetGamePlayState(Game::GamePlayState::GameOver);
    mDrawComponent->SetAnimation("dying");

    // Disable collider and rigid body
    mRigidBodyComponent->SetEnabled(false);
    mColliderComponent->SetEnabled(false);

    mGame->GetAudio()->StopAllSounds();
    // mGame->GetAudio()->PlaySound("Dead.wav");

    mGame->ResetGameScene(3.5f); // Reset the game scene after 3 seconds
}

void Punk::Win(AABBColliderComponent *poleCollider)
{
    mDrawComponent->SetAnimation("win");
    mGame->SetGamePlayState(Game::GamePlayState::LevelComplete);

    // Set Punk velocity to go down
    mRigidBodyComponent->SetVelocity(Vector2::UnitY * 100.0f); // 100 pixels per second
    mRigidBodyComponent->SetApplyGravity(false);

    // Disable collider
    poleCollider->SetEnabled(false);

    // Adjust Punk x position to grab the pole
    mPosition.Set(poleCollider->GetOwner()->GetPosition().x + Game::TILE_SIZE / 4.0f, mPosition.y);

    // Stop music
    mGame->GetAudio()->StopAllSounds();

    mPoleSlideTimer = POLE_SLIDE_TIME; // Start the pole slide timer
}

void Punk::OnHorizontalCollision(const float minOverlap, AABBColliderComponent *other)
{
    if (other->GetLayer() == ColliderLayer::Enemy)
    {
        TakeDamage();
        return;
    }

    if (other->GetLayer() == ColliderLayer::EnemyProjectile)
    {
        TakeDamage();
        other->GetOwner()->SetState(ActorState::Destroy);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Portal && mGame->GetGameScene() == Game::GameScene::Level1)
    {
        mGame->SetGameScene(Game::GameScene::Level2, .25f);
        other->SetEnabled(false);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Portal && mGame->GetGameScene() == Game::GameScene::Level2)
    {
        mGame->SetGameScene(Game::GameScene::Ending_GoHome, .25f);
        other->SetEnabled(false);
        return;
    }
    if (other->GetLayer() == ColliderLayer::Portal2 && mGame->GetGameScene() == Game::GameScene::Level2)
    {
        mGame->SetGameScene(Game::GameScene::Ending_Stay, .25f);
        other->SetEnabled(false);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Item)
    {
        other->GetOwner()->OnCollision();
    }
}

void Punk::OnVerticalCollision(const float minOverlap, AABBColliderComponent *other)
{
    if (other->GetLayer() == ColliderLayer::Enemy)
    {
        TakeDamage();
        return;
    }

    if (other->GetLayer() == ColliderLayer::EnemyProjectile)
    {
        TakeDamage();
        other->GetOwner()->SetState(ActorState::Destroy);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Portal && mGame->GetGameScene() == Game::GameScene::Level1)
    {
        mGame->SetGameScene(Game::GameScene::Level2, .25f);
        other->SetEnabled(false);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Portal && mGame->GetGameScene() == Game::GameScene::Level2)
    {
        mGame->SetGameScene(Game::GameScene::Ending_GoHome, .25f);
        other->SetEnabled(false);
        return;
    }
    if (other->GetLayer() == ColliderLayer::Portal2 && mGame->GetGameScene() == Game::GameScene::Level2)
    {
        mGame->SetGameScene(Game::GameScene::Ending_Stay, .25f);
        other->SetEnabled(false);
        return;
    }

    if (other->GetLayer() == ColliderLayer::Item)
    {
        other->GetOwner()->OnCollision();
    }
}

void Punk::FindKey()
{
    mFoundKey = true;
    mGame->GetAudio()->PlaySound("KeyPick.wav");

    if (mGame->GetGameScene() == Game::GameScene::Level1)
    {
        DialogueSystem::Get()->StartDialogue(
            {"Punk: Uma chave! Agora, o que sera que ela abre?",
             "Punk: Melhor eu dar uma olhada ao redor."},
            [this]()
            {
                mGame->SetGamePlayState(Game::GamePlayState::Playing);
            });
        const auto &portal = new Portal(mGame);
        portal->SetPosition(Vector2(622.0f, 210.0f));
    }
    else
    {
        DialogueSystem::Get()->StartDialogue(
            {"Voz: Voce conseguiu. A ultima chave foi encontrada.",
             "Voz: O caminho se abre em dois. O seu... e o nosso.",
             "Voz: O portal verde oferece o seu lar, a sua paz... ao custo da nossa existencia.",
             "Voz: O portal roxo vai te manter aqui, como o novo guardiao, para nos salvar.",
             "Voz: A escolha e sua, Viajante do Eter."},
            [this]()
            {
                mGame->SetGamePlayState(Game::GamePlayState::Playing);
            });
        const auto &portal = new Portal(mGame);
        portal->SetPosition(Vector2(288.0f, 992.0f));

        const auto &portal2 = new Portal(mGame, 1);
        portal2->SetPosition(Vector2(416.0f, 970.0f));
    }
}

void Punk::FindHeart()
{
    if (mLives < 6)
    {
        mLives++;
    }
    mGame->GetAudio()->PlaySound("KeyPick.wav");
}

void Punk::FindShotgun()
{
    mArm->ChangeWeapon();
    mArm->mFoundShotgun = true;
    mGame->GetAudio()->PlaySound("KeyPick.wav");
}

int Punk::GetAmmo()
{
    return mArm->mChosenWeapon->mAmmo;
}

int Punk::GetMaxAmmo()
{
    return mArm->mChosenWeapon->mMaxAmmo;
}

std::string Punk::GetCurrentWeaponName()
{
    if (!mArm->mChosenWeapon)
    {
        return "Unknown";
    }

    if (mArm->mChosenWeapon == mArm->mPistol)
    {
        return "Pistol";
    }
    else if (mArm->mChosenWeapon == mArm->mShotgun)
    {
        return "Shotgun";
    }

    return "Unknown";
}
