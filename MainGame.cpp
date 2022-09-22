#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"



#pragma region Globals
int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

#pragma endregion Global Variables

// enum of play states
enum Agent8State
{
	STATE_APPEAR = 0,
	STATE_HALT,
	STATE_PLAY,
	STATE_DEAD,
};


//struct to record game state information
struct GameState
{
	int score = 0;
	Agent8State agentState = STATE_APPEAR;
};
GameState gameState;

// enum of game object types
enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};

#pragma region declarations

void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinsAndStars();
void UpdateLasers();
void UpdateDestroyed();
void UpdateAgent8();

#pragma endregion all function declarations

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::CentreAllSpriteOrigins();
	Play::LoadBackground("Data\\Backgrounds\\background.png");
	Play::StartAudioLoop("music");

	Play::CreateGameObject(TYPE_AGENT8, { 115, 0 }, 50, "agent8");
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140, 217 }, 0, "fan");
	Play::GetGameObject(id_fan).velocity = { 0, 3 };
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	// draw background image
	Play::DrawBackground();

	// Update game objects
	UpdateAgent8();
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateDestroyed();

	// draw key and score
	Play::DrawFontText("64px", "ARROW KEYS MOVE UP AND DOWN, SPACE TO FIRE", { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 30 }, Play::CENTRE);
	Play::DrawFontText("132px", "SCORE: " + std::to_string(gameState.score), {DISPLAY_WIDTH / 2, 50}, Play::CENTRE);

	// draw current draw buffer
	Play::PresentDrawingBuffer();

	// return escape key
	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

/// <summary>
/// Update method for the fan object
/// </summary>
void UpdateFan()
{
	// get reference to fan
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);

	// generating projectiles 
	if (Play::RandomRoll(50) == 1)
	{
		// basic object creation stuff
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);

		// 1 in 3 chance of upwards, downwards, or stright accross directions
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);

		// 50/50 chance of a spanner instead of a screwdriver
		if (Play::RandomRoll(2) == 1)
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}
		Play::PlayAudio("tool");
	}

	// generating coinds
	if (Play::RandomRoll(150) == 1)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3,0 };
		obj_coin.rotSpeed = 0.1f;
	}

	// update fan objects new information.
	Play::UpdateGameObject(obj_fan);

	// check if the fan is moving too far up or down and reverse direction
	if (Play::IsLeavingDisplayArea(obj_fan))
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}

	// draw the fan
	Play::DrawObject(obj_fan);
}

/// <summary>
/// Update method for the coin and star objects
/// </summary>
void UpdateCoinsAndStars()
{
	// getting the player and all current coins
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	// loop coins
	for (int id_coin : vCoins)
	{

		GameObject& obj_coin = Play::GetGameObject(id_coin);
		bool hasCollided = false;

		// if the coin has collided with the player this frame
		if (Play::IsColliding(obj_coin, obj_agent8))
		{
			//for loop to generate stars in a circle around coin position 
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f)
			{
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = { 0, 0.5f };
				// set the star to move in a direction based on the current radion value
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			// reward player with score and sound effect.
			hasCollided = true;
			gameState.score += 500;
			Play::PlayAudio("collect");

		}
		// update and draw
		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		// if the coin is collected or off screen destroy it
		if (!Play::IsVisible(obj_coin) || hasCollided)
		{
			Play::DestroyGameObject(id_coin);
		}

	}

	// get all stars
	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	// loop stars
	for (int id_star : vStars)
	{
		// get reference, update, and draw
		GameObject& obj_star = Play::GetGameObject(id_star);
		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);
#
		// Destroy if off screen
		if (!Play::IsVisible(obj_star))
		{
			Play::DestroyGameObject(id_star);
		}

	}
}

/// <summary>
/// Update method for tool objects
/// </summary>
void UpdateTools()
{
	// get tool ids and player
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	// loop tools
	for (int id : vTools)
	{
		GameObject& obj_tool = Play::GetGameObject(id);
		
		// check for a collision with player
		if (gameState.agentState != STATE_DEAD && Play::IsColliding(obj_tool, obj_agent8))
		{
			// 'kill' player
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			gameState.agentState = STATE_DEAD;
		}

		// update tool object
		Play::UpdateGameObject(obj_tool);

		// if the tool hits the top of the screen, bounce back
		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}

		// draw tool
		Play::DrawObjectRotated(obj_tool);

		// if off screen, destroy object
		if (!Play::IsVisible(obj_tool))
		{
			Play::DestroyGameObject(id);
		}
	}
}

/// <summary>
/// Method for player controls
/// </summary>
void HandlePlayerControls()
{
	// get player object reference 
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	// check for up arrow press
	if (Play::KeyDown(VK_UP))
	{
		// move player up and player climb animation
		obj_agent8.velocity = { 0, -4 };
		Play::SetSprite(obj_agent8, "agent8_climb", 0.25f);
	}
	// check for down arrow press
	else if (Play::KeyDown(VK_DOWN))
	{
		// give player downwards acceleration, i.e. 'falling', player fall animation
		obj_agent8.acceleration = { 0, 1 };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
	}
	// if no user input, play hang animation and reduce velocity
	else
	{
		if (obj_agent8.velocity.y > 5)
		{
			gameState.agentState = STATE_HALT;
			Play::SetSprite(obj_agent8, "agent8_halt", 0.333f);
			obj_agent8.acceleration = { 0,0 };
		}
		else
		{
			Play::SetSprite(obj_agent8, "agent8_hang", 0.02f);
			obj_agent8.velocity *= 0.5f;
			obj_agent8.acceleration = { 0,0 };
		}

	}
	// check for spacebar input
	if (Play::KeyPressed(VK_SPACE))
	{
		// create laser to the right and up from the player centre
		Vector2D firePos = obj_agent8.pos + Vector2D(155, -75);
		int id = Play::CreateGameObject(TYPE_LASER, firePos, 30, "laser");

		// give laser velocity and play fire sound
		Play::GetGameObject(id).velocity = { 32, 0 };
		Play::PlayAudio("shoot");
	}

}

/// <summary>
/// Method for updating lasers
///</summary>
void UpdateLasers()
{
	// get ids for all lasers, tools, and coins
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	// for each laser id
	for (int id_laser : vLasers)
	{
		// get laser object and create boolean for collision
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;

		// for each tool
		for (int id_tool : vTools)
		{
			// get tool object and check for a collision with this laser
			GameObject& obj_tool = Play::GetGameObject(id_tool);
			if (Play::IsColliding(obj_laser, obj_tool))
			{
				// set tool to destroyed type and add score.
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score += 100;
			}
		}

		// for each coin
		for (int id_coin : vCoins)
		{
			// get coin object and check for collision with this laser
			GameObject& obj_coin = Play::GetGameObject(id_coin);
			if (Play::IsColliding(obj_laser, obj_coin))
			{
				// set coin type to destroyed, remove score and play audio to indicate a misplay
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
			}
		}

		// if the score goes below 0 reset to 0
		if (gameState.score < 0)
		{
			gameState.score = 0;
		}

		// update and draw laser object
		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);


		// destroy laser if off screen or has collided.
		if (!Play::IsVisible(obj_laser) || hasCollided)
		{
			Play::DestroyGameObject(id_laser);
		}

	}

}

/// <summary>
/// Method for updating 'destroyed' objects
/// </summary>
void UpdateDestroyed()
{
	// get all destroyed object ids
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	// for each destroyed object
	for (int id_dead : vDead)
	{
		// get object
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		
		// set object animation to low speed and update object
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		// only draw the object on odd numbered frames to create flashing effect
		if (obj_dead.frame % 2)
		{
			// draw objects with reduced opacity
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);
		}
		// destroy object when not visible or past frame 10 of animation
		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10)
		{
			Play::DestroyGameObject(id_dead);
		}
	}
}

/// <summary>
/// Update method for the player object
/// </summary>
void UpdateAgent8()
{
	// get player object reference 
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	
	// switch statement to act as state machine
	switch (gameState.agentState)
	{

	// starting state, make agent fall to part way down the screen
	case STATE_APPEAR:
		obj_agent8.velocity = { 0,12 };
		obj_agent8.acceleration = { 0, 0.5f };
		Play::SetSprite(obj_agent8, "agent8_fall", 0);
		obj_agent8.rotation = 0;
		if (obj_agent8.pos.y >= DISPLAY_HEIGHT / 3)
		{
			gameState.agentState = STATE_PLAY;
		}
		break;

	// state for if the player falls too far
	// Halts the player and sets the state to play after an animation
	case STATE_HALT:
		obj_agent8.velocity *= 0.9f;
		if (Play::IsAnimationComplete(obj_agent8))
		{
			gameState.agentState = STATE_PLAY;
		}
		break;

	// state for normal play, just calls the player controls method
	case STATE_PLAY:
		HandlePlayerControls();
		break;

	// state for if the player dies
	// if the space bar is pressed resets the player and sets state to appear.
	// changes all objects to be destroyed
	case STATE_DEAD:
		obj_agent8.acceleration = { -0.3f, 0.5f };
		obj_agent8.rotation += 0.25f;
		if (Play::KeyPressed(VK_SPACE) == true)
		{
			gameState.agentState = STATE_APPEAR;
			obj_agent8.pos = { 115,0 };
			obj_agent8.velocity = { 0,0 };
			obj_agent8.frame = 0;
			Play::StartAudioLoop("music");
			gameState.score = 0;

			for (int id_obj : Play::CollectGameObjectIDsByType(TYPE_TOOL))
			{
				Play::GetGameObject(id_obj).type = TYPE_DESTROYED;
			}
			
			for (int id_coin : Play::CollectGameObjectIDsByType(TYPE_COIN))
			{
				Play::GetGameObject(id_coin).type = TYPE_DESTROYED;
			}
			
		}
		break;
	}



	// update the player object
	Play::UpdateGameObject(obj_agent8);


	// prevent the player from moving off screen
	if (Play::IsLeavingDisplayArea(obj_agent8) && gameState.agentState != STATE_DEAD)
	{
		obj_agent8.pos == obj_agent8.oldPos;
	}

	// draw 'spider web' and player object
	Play::DrawLine({ obj_agent8.pos.x, 0 }, obj_agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(obj_agent8);
	//Play::DrawObject(obj_agent8);

}
