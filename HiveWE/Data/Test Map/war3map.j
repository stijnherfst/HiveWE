globals
unit udg_BlackHole= null
unit udg_Summon8= null
integer udg_loadCurrent= 0
group udg_QJCS_tempGroup= CreateGroup()
rect udg_EchoBlast= null
real udg_RealVar= 0
itemtype array udg_itemclass
string udg_removeSpellNumber= ""
boolean array udg_QJCS_AoE
unit udg_UnitVar= null
integer udg_loadAgility= 0
integer udg_removeI= 0
boolean udg_SaveLoad_SaveToDisk= false
real udg_RealVar2= 0
real udg_RealVar3= 0
integer udg_loadLumber= 0
boolean udg_itemLoadBool= false
unit udg_UnitVar2= null
location udg_PointVar= null
location udg_dontcopythis= null
unit udg_SuperHexUnit= null
unit udg_Summon1= null
unit udg_Summon2= null
dialog udg_SecretSam= DialogCreate()
rect udg_SunStorm= null
unit udg_glassrepair= null
button udg_no= null
quest udg_spiderquest= null
string array udg_abilityarray
button udg_GoldShroom= null
unit udg_Summon3= null
boolean udg_QJC_Gold= false
unit udg_PressureCaster= null
dialog udg_glassrepairdialog= DialogCreate()
unit udg_Summon4= null
unit udg_Summon5= null
string udg_LoadString= ""
unit udg_Summon6= null
integer udg_loadGold= 0
unit udg_Summon7= null
unit udg_Summon9= null
boolean array udg_QJCS_Enemy
button udg_MasterSecret= null
unit udg_Summon10= null
string udg_removeAbilitiesString= ""
unit udg_saveHero= null
timer udg_Arena= CreateTimer()
real udg_QJC_AoERadius= 0
boolean udg_validLoad= false
button udg_Glassswordrepair= null
string udg_saveTemp2= ""
integer udg_addPlayerNumber= 0
texttag udg_scottwings= null
string udg_LoadStringTemp= ""
integer udg_MerchantQuestReward= 0
real array udg_QJCS_AoERadius
boolean udg_QJC_Heatlh= false
button udg_SecretSummoning= null
integer udg_basicelemental= 0
string udg_removeAbilityName= ""
integer udg_removePlayerNumber= 0
integer udg_ArenaTimer= 0
integer udg_addi= 0
integer udg_loadExperience= 0
string udg_addString= ""
integer udg_toxicspider= 0
integer udg_lavarunner= 0
string udg_saveTemp= ""
integer udg_rndTemp= 0
integer udg_loadTemp= 0
quest udg_basicquest= null
quest udg_lavaquest= null
boolean array udg_playerLoaded
button udg_SecretSeals= null
integer udg_loadStrength= 0
integer udg_loadIntelligence= 0
integer udg_saveTempInt= 0
unit udg_ArenaBrawler= null
timer udg_Bacontimer= CreateTimer()
timerdialog udg_bacontimerwindow= null
timer udg_Armageddon= CreateTimer()
timerdialog udg_ArenaTimerWindow= null
timerdialog udg_Armagedontimerwindow= null
player udg_ChallengePlayer= null
integer udg_SaveLoad_HeroCount= 0
player udg_AcceptPlayer= null
unit udg_Champion= null
integer udg_ArenaRnd= 0
unit udg_ArenaMonster= null
integer udg_presurerelaese= 0
unit udg_pressuretarget= null
location udg_BlackHolePoint= null
unit udg_BlackHoleSuckUnits= null
boolean array udg_QJCS_Stun
integer udg_raremonsterhp= 0
unit udg_LitFurytarget= null
unit udg_LitFuryowner= null
item array udg_CleanedItem
boolean udg_QJC_Enemy= false
boolean udg_ItemCleanupFlag= false
timer udg_ItemCleanupTimer= CreateTimer()
boolean array udg_QJCS_Health
integer udg_ItemsToClean= 0
integer udg_Loop= 0
location udg_Point= null
string udg_SaveLoad_Directory= ""
string udg_SaveLoad_Alphabet= ""
boolean udg_SaveLoad_CheckName= false
boolean udg_SaveLoad_Security= false
integer udg_SaveLoad_HyphenSpace= 0
attacktype array udg_QJCS_AttackType
string udg_SaveLoad_SeperationChar= ""
string udg_SaveLoad_Lower= ""
string udg_SaveLoad_Number= ""
string udg_SaveLoad_Upper= ""
integer udg_SaveLoad_MaxValue= 0
string udg_SaveLoad_Error= ""
integer array udg_SaveLoad_Hero
integer array udg_SaveLoad_Abilities
integer udg_SaveLoad_AbilityCount= 0
integer array udg_SaveLoad_Item
integer udg_SaveLoad_ItemCount= 0
string udg_SaveLoad_Full= ""
real array udg_QJCS_ChainZ
integer udg_SaveLoad_Base= 0
string array udg_SaveLoad_Char
integer array udg_Load
integer udg_LoadCount= 0
integer udg_SaveCount= 0
unit udg_Hero= null
integer array udg_Save
item udg_Item= null
timerdialog udg_Volcano_Timer_Window= null
string udg_SaveLoad_Filename= ""
string udg_Code= ""
boolean udg_SaveLoad_Valid= false
integer array udg_QJCS_index
integer udg_QJC_JumpCount= 0
location array udg_QJCS_tempPos
unit udg_QJC_Caster= null
real udg_QJC_JumpRadius= 0
location udg_QJC_TargetPoint= null
integer udg_QJCS_Locust= 0
boolean udg_QJC_Ally= false
unit udg_QJC_TargetUnit= null
boolean udg_QJC_NoTarget= false
real udg_QJC_AmountReduce= 0
real udg_QJC_JumpDelayTime= 0
unit array udg_QJCS_Caster
unit array udg_QJCS_TargetUnit
boolean array udg_QJCS_Priority
boolean udg_QJC_Priority= false
boolean array udg_QJCS_OnePerUnit
boolean udg_QJC_OnePerUnit= false
boolean array udg_QJCS_Damage
boolean udg_QJC_Damage= false
boolean array udg_QJCS_Heal
boolean udg_QJC_Heal= false
boolean array udg_QJCS_Mana
boolean udg_QJC_Mana= false
boolean array udg_QJCS_Gold
boolean array udg_QJCS_Leech
boolean udg_QJC_Leech= false
boolean array udg_QJCS_Ally
location array udg_QJCS_LightningPos
real array udg_QJCS_Amount
integer udg_QJC_SlowEffect= 0
real udg_QJC_Amount= 0
attacktype udg_QJC_AttackType= null
integer udg_QJC_DummyType= 0
damagetype array udg_QJCS_DamageType
damagetype udg_QJC_DamageType= null
real array udg_QJCS_AmountReduce
integer array udg_QJCS_JumpCount
real array udg_QJCS_JumpDelayTime
real array udg_QJCS_JumpRadius
boolean udg_QJC_AoE= false
string array udg_QJCS_ChainSFX
string udg_QJC_ChainSFX= ""
string array udg_QJCS_TargetSFX
string udg_QJC_TargetSFX= ""
boolean array udg_QJCS_Slow
boolean udg_QJC_Slow= false
integer array udg_QJCS_SlowEffect
boolean udg_QJC_Stun= false
integer array udg_QJCS_StunEffect
integer udg_QJC_StunEffect= 0
integer array udg_QJCS_SystemCount
real array udg_QJCS_SystemTime
unit array udg_QJCS_baseDummy
group udg_QJCS_dummyGroup= CreateGroup()
group array udg_QJCS_dGroup
unit array udg_QJCS_prevTarget
integer array udg_QJCS_LightningIndex
unit array udg_QJCS_LightningTarget
real array udg_QJCS_LightningDur
unit array udg_QJCS_LightningDPos
real array udg_QJCS_ChainX
real array udg_QJCS_ChainY
group array udg_QJCS_aGroup
group udg_QJCS_victimGroup= CreateGroup()
unit udg_QJCS_currUnit= null
lightning array udg_QJCS_LightningSFX
location udg_dontcopy_tempPos= null
integer udg_furbolgdigger= 0
quest udg_furbolgquest= null
string array udg_stringParts
integer udg_stringPartsCount= 0
timer udg_VolcanoTimer= CreateTimer()
location udg_VolcanoPoint= null
unit array udg_Summoned_Unit
trigger gg_trg_Player_Leaves= null
trigger gg_trg_Inventory= null
trigger gg_trg_Becoming_a_Master= null

	

// processed: 	integer array playerAbilities[71]

integer array abilities

integer array summoningAbilities

integer summoningAbilitiesCount= 24

	

	

integer array typeToFortified

integer array typeToDivine

integer array typeToFlying



//JASSHelper struct globals:
integer array s__playerAbilities

endglobals


//===========================================================================
//*
//*  Global variables
//*
//===========================================================================

function InitGlobals takes nothing returns nothing
 local integer i= 0
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_AoE[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_abilityarray[i]=""
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Enemy[i]=false
		set i=i + 1
	endloop
	set udg_MerchantQuestReward=4
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_AoERadius[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 10 )
		set udg_playerLoaded[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Stun[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Health[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_SaveLoad_Hero[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_SaveLoad_Abilities[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_SaveLoad_Item[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_ChainZ[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_SaveLoad_Char[i]=""
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_Load[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_Save[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_index[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Priority[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_OnePerUnit[i]=true
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Damage[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Heal[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Mana[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Gold[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Leech[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Ally[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Amount[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_AmountReduce[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_JumpCount[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_JumpDelayTime[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_JumpRadius[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_ChainSFX[i]=""
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_TargetSFX[i]=""
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_Slow[i]=false
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_SlowEffect[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_StunEffect[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_SystemCount[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_SystemTime[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_dGroup[i]=CreateGroup()
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_LightningIndex[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_LightningDur[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_ChainX[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_ChainY[i]=0
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 1 )
		set udg_QJCS_aGroup[i]=CreateGroup()
		set i=i + 1
	endloop
	set i=0
	loop
		exitwhen ( i > 50 )
		set udg_stringParts[i]=""
		set i=i + 1
	endloop
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



function filter_unit_is_hero takes nothing returns boolean

    return IsUnitType(GetFilterUnit(), UNIT_TYPE_HERO) == true

endfunction



function CreateTextFileForPlayer takes player pl returns nothing

 local unit hero= null

 local group hero_group= CreateGroup()

    local boolexpr filter= Filter(function filter_unit_is_hero)

 local string fileData= ""

 local string fileName= ""



    // Get Hero from the units owned by the player

    call GroupEnumUnitsOfPlayer(hero_group, pl, filter)

    call DestroyBoolExpr(filter)

    set hero=FirstOfGroup(hero_group)

    set hero_group=null

    set filter=null



	// Right now, this is:

	//      Hero: (hero name)

	//      Level: (hero level)

	//      Code: -load XXXX

	set fileData="\r\n\t\t\t\tHero: " + GetUnitName(hero) + "\r\n\t\t\t\tLevel: " + I2S(GetHeroLevel(hero)) + "\t\t\r\n\t\t\t\tCode: -l " + udg_saveTemp + "\r\n\n\t\t    "



	// Right now, this is:

	//      "Warcraft III\FolderName\(hero name) - (hero level)"

	set fileName="MCFC\\" + GetUnitName(hero) + " - " + I2S(GetHeroLevel(hero)) + ".txt"



	if GetLocalPlayer() == pl then

	   call PreloadGenClear()

	   call PreloadGenStart()



	   call Preload(fileData)

	   call PreloadGenEnd(fileName)

	endif

endfunction



function at takes string text,integer position returns string

	return SubString(text, position, position + 1)

endfunction



function search_in_string takes string text,string character returns integer

 local integer i= 0

    loop

        exitwhen i > StringLength(text) - 1

        if ( character == at(text , i) ) then

			return i

		endif

        set i=i + 1

    endloop

	return - 1

endfunction



function split takes string text,string seperator returns nothing

 local integer j= 0

	set udg_stringParts[0]=""

	set udg_stringPartsCount=0

    loop

        exitwhen j > StringLength(text) - 1

        if ( seperator == at(text , j) ) then

			set udg_stringPartsCount=udg_stringPartsCount + 1

			set udg_stringParts[udg_stringPartsCount]=""

		else

			set udg_stringParts[udg_stringPartsCount]=udg_stringParts[udg_stringPartsCount] + at(text , j)

		endif

        set j=j + 1

    endloop

endfunction



function UnitHasItemOfType takes unit unit_to_check,itemtype type_of_item returns integer

 local integer index

 local item indexItem

 local integer count= 0

	

	set index=0

	loop

		exitwhen index >= bj_MAX_INVENTORY

		set indexItem=UnitItemInSlot(unit_to_check, index)

		if ( indexItem != null ) and ( GetItemType(indexItem) == type_of_item ) then

			set count=count + 1

		endif

		set index=index + 1

	endloop

	return count

endfunction



function PlayerHasSpell takes player whichPlayer,integer spell returns boolean

 local integer i= 0

	loop

		exitwhen i > 6

		if ( s__playerAbilities[GetPlayerId(whichPlayer) * 7 + i] == spell ) then

			return true

		endif

		set i=i + 1

	endloop

	return false

endfunction//===========================================================================
//*
//*  Triggers
//*
//===========================================================================
function Trig_Player_Leaves_Remove takes nothing returns nothing

    call RemoveUnit(GetEnumUnit())

endfunction



function Trig_Player_Leaves_Actions takes nothing returns nothing

    local group g= CreateGroup()

    

    call DisplayTimedTextToPlayer(GetLocalPlayer(), 0, 0, 25.00, GetPlayerName(GetTriggerPlayer()) + " has left the game.")

    call GroupEnumUnitsOfPlayer(g, GetTriggerPlayer(), null)

    call ForGroup(g, function Trig_Player_Leaves_Remove)

    

    call DestroyGroup(g)

    set g=null

endfunction



//===========================================================================

function InitTrig_Player_Leaves takes nothing returns nothing

    local trigger l__gg_trg_Player_Leaves= CreateTrigger()

    local integer i= 0

	loop

	    exitwhen ( i > 9 )

        call TriggerRegisterPlayerEvent(l__gg_trg_Player_Leaves, Player(i), EVENT_PLAYER_LEAVE)

	    set i=i + 1

	endloop

	call TriggerAddAction(l__gg_trg_Player_Leaves, function Trig_Player_Leaves_Actions)

    set l__gg_trg_Player_Leaves=null

endfunction




//===========================================================================
// Trigger: Inventory
//===========================================================================
function Trig_Inventory_Actions takes nothing returns nothing
	call DisplayTimedTextToForce(GetPlayersAll(), 60.00, "TRIGSTR_046")
endfunction

//===========================================================================
function InitTrig_Inventory takes nothing returns nothing
	set gg_trg_Inventory=CreateTrigger()
	call TriggerRegisterTimerEventSingle(gg_trg_Inventory, 360.00)
	call TriggerAddAction(gg_trg_Inventory, function Trig_Inventory_Actions)
endfunction

//===========================================================================
// Trigger: Becoming_a_Master
//===========================================================================
function Trig_Becoming_a_Master_Actions takes nothing returns nothing
	call DisplayTimedTextToForce(GetPlayersAll(), 50.00, "TRIGSTR_047")
endfunction

//===========================================================================
function InitTrig_Becoming_a_Master takes nothing returns nothing
	set gg_trg_Becoming_a_Master=CreateTrigger()
	call TriggerRegisterTimerEventSingle(gg_trg_Becoming_a_Master, 500.00)
	call TriggerAddAction(gg_trg_Becoming_a_Master, function Trig_Becoming_a_Master_Actions)
endfunction

//===========================================================================
function InitCustomTriggers takes nothing returns nothing
	call InitTrig_Player_Leaves()
	call InitTrig_Inventory()
	call InitTrig_Becoming_a_Master()
endfunction
//===========================================================================
function RunInitializationTriggers takes nothing returns nothing
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
	call SetCameraBounds(- 4096.000000 + GetCameraMargin(CAMERA_MARGIN_LEFT), - 4096.000000 + GetCameraMargin(CAMERA_MARGIN_BOTTOM), 4096.000000 - GetCameraMargin(CAMERA_MARGIN_RIGHT), 4096.000000 - GetCameraMargin(CAMERA_MARGIN_TOP), - 4096.000000 + GetCameraMargin(CAMERA_MARGIN_LEFT), 4096.000000 - GetCameraMargin(CAMERA_MARGIN_TOP), 4096.000000 - GetCameraMargin(CAMERA_MARGIN_RIGHT), - 4096.000000 + GetCameraMargin(CAMERA_MARGIN_BOTTOM))
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
	call InitCustomTriggers()
	call RunInitializationTriggers()
endfunction
//===========================================================================
//*
//*  Map Configuration
//*
//===========================================================================
function config takes nothing returns nothing
	call SetMapName("TRIGSTR_004 ")
	call SetMapDescription("TRIGSTR_006 ")
	call SetPlayers(1)
	call SetTeams(1)
	call SetGamePlacement(MAP_PLACEMENT_TEAMS_TOGETHER)

	call DefineStartLocation(0, - 192.000000, 2176.000000)

	call InitCustomPlayerSlots()
	call SetPlayerTeam(Player(0), 0) // INLINED!!
	call SetStartLocPrioCount(0, 0) // INLINED!!
endfunction



//Struct method generated initializers/callers:

//Functions for BigArrays:

