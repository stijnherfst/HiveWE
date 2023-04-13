globals
trigger gg_trg_Melee_Initialization= null


//JASSHelper struct globals:

endglobals


//===========================================================================
//*
//*  Global variables
//*
//===========================================================================

function InitGlobals takes nothing returns nothing
 local integer i= 0
endfunction

//===========================================================================
//*
//*  Map Item Tables
//*
//===========================================================================
//===========================================================================
//*
//*  Unit Item Tables
//*
//===========================================================================
//===========================================================================
//*
//*  Sounds
//*
//===========================================================================
function InitSounds takes nothing returns nothing
endfunction
//===========================================================================
//*
//*  Destructable Objects
//*
//===========================================================================
function CreateDestructables takes nothing returns nothing
 local destructable d
 local trigger t
 local real life
endfunction
//===========================================================================
//*
//*  Items
//*
//===========================================================================
function CreateItems takes nothing returns nothing
 local integer itemID
endfunction
//===========================================================================
//*
//*  Unit Creation
//*
//===========================================================================
function CreateUnits takes nothing returns nothing
 local unit u
 local integer unitID
 local trigger t
 local real life
	set u=BlzCreateUnitWithSkin(Player(0), 'nhea', - 906.2883, - 432.4324, 262.5669, 'nhea')
endfunction
//===========================================================================
//*
//*  Regions
//*
//===========================================================================
function CreateRegions takes nothing returns nothing
 local weathereffect we

endfunction
//===========================================================================
//*
//*  Cameras
//*
//===========================================================================
function CreateCameras takes nothing returns nothing
endfunction
//===========================================================================
//*
//*  Custom Script Code
//*
//===========================================================================
//===========================================================================
//*
//*  Triggers
//*
//===========================================================================
//===========================================================================
// Trigger: Melee_Initialization
//===========================================================================
function Trig_Melee_Initialization_Actions takes nothing returns nothing
	call MeleeStartingVisibility()
	call MeleeStartingHeroLimit()
	call MeleeGrantHeroItems()
	call MeleeStartingResources()
	call MeleeClearExcessUnits()
	call MeleeStartingUnits()
	call MeleeStartingAI()
	call MeleeInitVictoryDefeat()
endfunction

//===========================================================================
function InitTrig_Melee_Initialization takes nothing returns nothing
	set gg_trg_Melee_Initialization=CreateTrigger()
	call TriggerAddAction(gg_trg_Melee_Initialization, function Trig_Melee_Initialization_Actions)
endfunction

//===========================================================================
function InitCustomTriggers takes nothing returns nothing
	call InitTrig_Melee_Initialization()
endfunction
//===========================================================================
function RunInitializationTriggers takes nothing returns nothing
	call ConditionalTriggerExecute(gg_trg_Melee_Initialization)
endfunction
//===========================================================================
//*
//*  Players
//*
//===========================================================================
function InitCustomPlayerSlots takes nothing returns nothing
	call SetPlayerStartLocation(Player(0), 0)
	call SetPlayerColor(Player(0), ConvertPlayerColor(0))
	call SetPlayerRacePreference(Player(0), RACE_PREF_HUMAN)
	call SetPlayerRaceSelectable(Player(0), true)
	call SetPlayerController(Player(0), MAP_CONTROL_USER)

endfunction

function InitCustomTeams takes nothing returns nothing

	// Force: TRIGSTR_002
	call SetPlayerTeam(Player(0), 0)

endfunction
function InitAllyPriorities takes nothing returns nothing
	call SetStartLocPrioCount(0, 0)
endfunction
//===========================================================================
//*
//*  Main Initialization
//*
//===========================================================================
function main takes nothing returns nothing
	call SetCameraBounds(- 3328.000000 + GetCameraMargin(CAMERA_MARGIN_LEFT), - 3584.000000 + GetCameraMargin(CAMERA_MARGIN_BOTTOM), 3328.000000 - GetCameraMargin(CAMERA_MARGIN_RIGHT), 3072.000000 - GetCameraMargin(CAMERA_MARGIN_TOP), - 3328.000000 + GetCameraMargin(CAMERA_MARGIN_LEFT), 3072.000000 - GetCameraMargin(CAMERA_MARGIN_TOP), 3328.000000 - GetCameraMargin(CAMERA_MARGIN_RIGHT), - 3584.000000 + GetCameraMargin(CAMERA_MARGIN_BOTTOM))
	call SetDayNightModels("Environment/DNC/DNCLordaeron/DNCLordaeronTerrain/DNCLordaeronTerrain.mdl", "Environment/DNC/DNCLordaeron/DNCLordaeronTerrain/DNCLordaeronTerrain.mdl")
	call NewSoundEnvironment("Default")
	call SetAmbientDaySound("LordaeronSummerDay")
	call SetAmbientNightSound("LordaeronSummerNight")
	call SetMapMusic("Music", true, 0)
	call InitSounds()
	call CreateRegions()
	call CreateCameras()
	call CreateDestructables()
	call CreateItems()
	call CreateUnits()
	call InitBlizzard()


	call InitGlobals()
	call InitTrig_Melee_Initialization() // INLINED!!
	call ConditionalTriggerExecute(gg_trg_Melee_Initialization) // INLINED!!
endfunction
//===========================================================================
//*
//*  Map Configuration
//*
//===========================================================================
function config takes nothing returns nothing
	call SetMapName("Just another Warcraft III map ")
	call SetMapDescription("Nondescript ")
	call SetPlayers(1)
	call SetTeams(1)
	call SetGamePlacement(MAP_PLACEMENT_TEAMS_TOGETHER)

	call DefineStartLocation(0, - 1856, - 768)

	call InitCustomPlayerSlots()
	call SetPlayerSlotAvailable(Player(0), MAP_CONTROL_USER)
	call InitGenericPlayerSlots()
	call SetStartLocPrioCount(0, 0) // INLINED!!
endfunction



//Struct method generated initializers/callers:

