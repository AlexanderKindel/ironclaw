typedef struct Skill
{
    char*name;
    uint8_t mark_count;
} Skill;

Skill g_skills[] = { { "Academics" }, { "Brawling" }, { "Climbing" }, { "Craft" }, { "Deceit" },
{ "Digging" }, { "Dodge" }, { "Endurance" }, { "Gossip" }, { "Inquiry" }, { "Jumping" },
{ "Leadership" }, { "Mle Combat" }, { "Negotiation" }, { "Observation" }, { "Presence" },
{ "Ranged Combat" }, { "Riding" }, { "Searching" }, { "Stealth" }, { "Supernatural" },
{ "Swimming" }, { "Tactics" }, { "Throwing" }, { "Vehicles" }, { "Weather Sense" } };

#define DIE_DENOMINATIONS(macro)\
macro(DENOMINATION_D4, ("D4"))\
macro(DENOMINATION_D6, ("D6"))\
macro(DENOMINATION_D8, ("D8"))\
macro(DENOMINATION_D10, ("D10"))\
macro(DENOMINATION_D12, ("D12"))

enum DieDenominationIndex
{
    DIE_DENOMINATIONS(MAKE_ENUM)
};

char*g_die_denominations[] = { DIE_DENOMINATIONS(MAKE_VALUE) };

typedef struct Trait
{
    char*name;
    uint8_t dice[ARRAY_COUNT(g_unallocated_dice)];
} Trait;

#define TRAITS(macro)\
macro(TRAIT_BODY, ({ "Body" }))\
macro(TRAIT_SPEED, ({ "Speed" }))\
macro(TRAIT_MIND, ({ "Mind" }))\
macro(TRAIT_WILL, ({ "Will" }))\
macro(TRAIT_SPECIES, ({ "Species" }))\
macro(TRAIT_CAREER, ({ "Career" }))

enum TraitIndex
{
    TRAITS(MAKE_ENUM)
};

Trait g_traits[] = { TRAITS(MAKE_VALUE) };

enum GiftDescriptor
{
    DESCRIPTOR_INFLUENCE = 0b1,
    DESCRIPTOR_MULTIPLE = 0b10
};

enum RequirementIndex
{
    REQUIREMENT_TEXT_JUSTIFICATION,
    REQUIREMENT_GIFT,
    REQUIREMENT_NO_GIFT_WITH_DESCRIPTOR,
    REQUIREMENT_TRAIT,
    REQUIREMENT_CAREER_GIFTS,
    REQUIREMENT_FAVORITE_USE,
    REQUIREMENT_HOST_PERMISSION
};

typedef struct Requirement
{
    union
    {
        char*justification;
        uint16_t descriptor_flags;
        uint8_t gift_index;
        struct
        {
            uint8_t trait_index;
            uint8_t minimum_denomination;
        };
    };
    uint8_t requirement_type;
} Requirement;

typedef struct Gift
{
    char*name;
    Requirement*requirements;
    uint16_t descriptor_flags;
    uint8_t requirement_count;
} Gift;

#define GIFTS(macro)\
macro(GIFT_ACROBAT, ({ "Acrobat" }))\
macro(GIFT_BRAWLING_FIGHTER, ({ "Brawling Fighter" }))\
macro(GIFT_CHARGING_STRIKE, ({ "Charging Strike" }))\
macro(GIFT_CONTORTIONIST, ({ "Contortionist" }))\
macro(GIFT_COWARD, ({ "Coward" }))\
macro(GIFT_FAST_CLIMBER, ({ "Fast Climber" }))\
macro(GIFT_FAST_JUMPER, ({ "Fast Jumper" }))\
macro(GIFT_FAST_SWIMMER, ({ "Fast Swimmer" }))\
macro(GIFT_FRENZY, ({ "Frenzy" }))\
macro(GIFT_GIANT,\
({\
    "Giant",\
    (Requirement[])\
    {\
        {\
            .trait_index = TRAIT_BODY,\
            .minimum_denomination = DENOMINATION_D12,\
            .requirement_type = REQUIREMENT_TRAIT\
        }\
    },\
    DESCRIPTOR_INFLUENCE, 1\
}))\
macro(GIFT_HIKING, ({ "Hiking" }))\
macro(GIFT_KEEN_EARS, ({ "Keen Ears" }))\
macro(GIFT_KEEN_EYES, ({ "Keen Eyes" }))\
macro(GIFT_KEEN_NOSE, ({ "Keen Nose" }))\
macro(GIFT_LEGERDEMAIN, ({ "Legerdemain" }))\
macro(GIFT_MELEE_FINESSE, ({ "Melee Finesse" }))\
macro(GIFT_MOUNTED_FIGHTER, ({ "Mounted Fighter" }))\
macro(GIFT_NIGHT_VISION, ({ "Night Vision" }))\
macro(GIFT_PACIFIST, ({ "Pacifist" }))\
macro(GIFT_PARKOUR,\
({\
    "Parkour",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FAST_CLIMBER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SPRINGING_STRIKE, ({ "Springing Strike" }))\
macro(GIFT_SURE_FOOTED, ({ "Sure-Footed" }))\
macro(GIFT_ANIMAL_HANDLING, ({ "Animal Handling" }))\
macro(GIFT_ARTIST, ({ "Artist" }))\
macro(GIFT_CLEAR_HEADED, ({ "Clear-Headed" }))\
macro(GIFT_CRAFT_SPECIALTY, ({ "Craft Specialty" }))\
macro(GIFT_DEAD_RECKONING, ({ "Dead Reckoning" }))\
macro(GIFT_EXTRA_FAVORITE, ({ "Extra Favorite" }))\
macro(GIFT_FIRST_AID, ({ "First Aid" }))\
macro(GIFT_GAMBLING, ({ "Gambling" }))\
macro(GIFT_GEOGRAPHY, ({ "Geography" }))\
macro(GIFT_HERALDRY, ({ "Heraldry" }))\
macro(GIFT_HISTORY, ({ "History" }))\
macro(GIFT_JUNK_EXPERT, ({ "Junk Expert" }))\
macro(GIFT_LANGUAGE, ({ "Language" }))\
macro(GIFT_MEDICINE, ({ "Medicine" }))\
macro(GIFT_MELEE_FERVOR, ({ "Melee Fervor" }))\
macro(GIFT_MELEE_GUILE, ({ "Melee Guile" }))\
macro(GIFT_MYSTIC, ({ "Mystic" }))\
macro(GIFT_OVERCONFIDENCE, ({ "Overconfidence" }))\
macro(GIFT_PACK_TACTICS, ({ "Pack Tactics" }))\
macro(GIFT_PIETY, ({ "Piety" }))\
macro(GIFT_SAILING, ({ "Sailing" }))\
macro(GIFT_SPELUNKING, ({ "Spelunking" }))\
macro(GIFT_TEAMSTER, ({ "Teamster" }))\
macro(GIFT_TRACKING, ({ "Tracking" }))\
macro(GIFT_UNSHAKEABLE_FIGHTER, ({ "Unshakeable Fighter" }))\
macro(GIFT_VENGEFUL_FIGHTER, ({ "Vengeful Fighter" }))\
macro(GIFT_BRIBERY, ({ "Bribery" }))\
macro(GIFT_CAROUSING, ({ "Carousing" }))\
macro(GIFT_COSMOPOLITAN, ({ "Cosmopolitan" }))\
macro(GIFT_DIPLOMACY, ({ "Diplomacy" }))\
macro(GIFT_DISGUISE, ({ "Disguise" }))\
macro(GIFT_ETIQUETTE, ({ "Etiquette" }))\
macro(GIFT_FAST_TALK, ({ "Fast-Talk" }))\
macro(GIFT_HAGGLING, ({ "Haggling" }))\
macro(GIFT_HONOR, ({ "Honor" }))\
macro(GIFT_INSIDER, ({ "Insider" }))\
macro(GIFT_LAW, ({ "Law" }))\
macro(GIFT_LEGAL_AUTHORITY,\
({\
    "Legal Authority",\
    (Requirement[])\
    {\
        {\
            .justification = "Authorization from a recognized legal authority.",\
            .requirement_type = REQUIREMENT_TEXT_JUSTIFICATION\
        }\
    },\
    DESCRIPTOR_INFLUENCE, 1\
}))\
macro(GIFT_LOCAL_KNOWLEDGE, ({ "Local Knowledge" }))\
macro(GIFT_LOW_PROFILE, ({ "Low Profile" }))\
macro(GIFT_NOBILITY,\
({\
    "Nobility",\
    (Requirement[])\
    {\
        {\
            .justification = "A noble bloodline or parent of nobility, traceable to a Great House or Minor House.",\
            .requirement_type = REQUIREMENT_TEXT_JUSTIFICATION\
        }\
    },\
    DESCRIPTOR_INFLUENCE | DESCRIPTOR_MULTIPLE, 1\
}))\
macro(GIFT_ORATORY, ({ "Oratory" }))\
macro(GIFT_ORDAINMENT,\
({\
    "Ordainment",\
    (Requirement[])\
    {\
        {\
            .justification = "An endorsement from a religious organization.",\
            .requirement_type = REQUIREMENT_TEXT_JUSTIFICATION\
        }\
    },\
    DESCRIPTOR_INFLUENCE | DESCRIPTOR_MULTIPLE, 1\
}))\
macro(GIFT_PERFORMANCE, ({ "Performance" }))\
macro(GIFT_SEDUCTION, ({ "Seduction" }))\
macro(GIFT_SHADOWING, ({ "Shadowing" }))\
macro(GIFT_SURVIVAL, ({ "Survival" }))\
macro(GIFT_TEAM_PLAYER, ({ "Team Player" }))\
macro(GIFT_WEALTH,\
({\
    "Wealth",\
    (Requirement[])\
    {\
        {\
            .justification = "An appropriate background and career.",\
            .requirement_type = REQUIREMENT_TEXT_JUSTIFICATION\
        }\
    },\
    DESCRIPTOR_INFLUENCE | DESCRIPTOR_MULTIPLE, 1\
}))\
macro(GIFT_ARCHERS_TRAPPINGS, ({ "Archer's Trappings" }))\
macro(GIFT_CLERICS_TRAPPINGS, ({ "Cleric's Trappings" }))\
macro(GIFT_COGNOSCENTES_TRAPPINGS, ({ "Cognoscente's Trappings" }))\
macro(GIFT_DILETTANTES_TRAPPINGS, ({ "Dilettante's Trappings" }))\
macro(GIFT_ELEMENTALISTS_TRAPPINGS, ({ "Elementalist's Trappings" }))\
macro(GIFT_FUSILEERS_TRAPPINGS, ({ "Fusileer's Trappings" }))\
macro(GIFT_KNIGHTS_TRAPPINGS, ({ "Knight's Trappings" }))\
macro(GIFT_MUSKETEERS_TRAPPINGS, ({ "Musketeer's Trappings" }))\
macro(GIFT_RIDERS_TRAPPINGS, ({ "Rider's Trappings" }))\
macro(GIFT_SCHOLARS_TRAPPINGS, ({ "Scholar's Trappings" }))\
macro(GIFT_SIGNATURE_ITEM, ({ "Signature Item" }))\
macro(GIFT_SPYS_TRAPPINGS, ({ "Spy's Trappings" }))\
macro(GIFT_THAUMATURGES_TRAPPINGS, ({ "Thaumaturge's Trappings" }))\
macro(GIFT_COMBAT_EDGE, ({ "Combat Edge" }))\
macro(GIFT_COMBAT_SAVE, ({ "Combat Save" }))\
macro(GIFT_DIEHARD,\
({\
    "Diehard",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_TOUGHNESS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_DISARMING_SAVE,\
({\
    "Disarming Save",\
    (Requirement[])\
    {\
        {\
            .trait_index = TRAIT_BODY,\
            .minimum_denomination = DENOMINATION_D8,\
            .requirement_type = REQUIREMENT_TRAIT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_DRAMATIC_DISHEVELING,\
({\
    "Dramatic Disheveling",\
    (Requirement[])\
    {\
        {\
            .trait_index = TRAIT_WILL,\
            .minimum_denomination = DENOMINATION_D8,\
            .requirement_type = REQUIREMENT_TRAIT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MAGIC_SAVE,\
({\
    "Magic Save",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_MYSTIC,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_REPLAY_FOR_DESTINY,\
({\
    "Replay for Destiny",\
    (Requirement[])\
    {\
        {\
            .trait_index = TRAIT_BODY,\
            .minimum_denomination = DENOMINATION_D8,\
        },\
        {\
            .trait_index = TRAIT_SPEED,\
            .minimum_denomination = DENOMINATION_D8,\
        },\
        {\
            .trait_index = TRAIT_MIND,\
            .minimum_denomination = DENOMINATION_D8,\
        },\
        {\
            .trait_index = TRAIT_WILL,\
            .minimum_denomination = DENOMINATION_D8,\
        }\
    },\
    0, 4\
}))\
macro(GIFT_RETREATING_SAVE,\
({\
    "Retreating Save",\
    (Requirement[])\
    {\
        {\
            .trait_index = TRAIT_SPEED,\
            .minimum_denomination = DENOMINATION_D8,\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SHIELD_SAVE,\
({\
    "Shield Save",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        },\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 2\
}))\
macro(GIFT_TOUGHNESS, ({ "Toughness", 0, DESCRIPTOR_MULTIPLE }))\
macro(GIFT_EXTRA_CAREER,\
({\
    "Extra Career",\
    (Requirement[])\
    {\
        {\
            .requirement_type = REQUIREMENT_CAREER_GIFTS\
        }\
    },\
    0, 1\
}))\
macro(GIFT_FAVOR_BONUS,\
({\
    "Favor Bonus",\
    (Requirement[])\
    {\
        {\
            .requirement_type = REQUIREMENT_FAVORITE_USE\
        }\
    },\
    0, 1\
}))\
macro(GIFT_INCREASED_TRAIT, ({ "Increased Trait", 0, DESCRIPTOR_MULTIPLE }))\
macro(GIFT_KNACK, ({ "Knack" }))\
macro(GIFT_LUCK, ({ "Luck", 0, DESCRIPTOR_MULTIPLE }))\
macro(GIFT_PERSONALITY, ({ "Personality" }))\
macro(GIFT_DEEP_DIVING, ({ "Deep Diving" }))\
macro(GIFT_ECHOLOCATION, ({ "Echolocation" }))\
macro(GIFT_FLIGHT, ({ "Flight" }))\
macro(GIFT_HOWLING, ({ "Howling" }))\
macro(GIFT_NATURAL_ARMOR, ({ "Natural Armor" }))\
macro(GIFT_PREHENSILE_FEET, ({ "Prehensile Feet" }))\
macro(GIFT_PREHENSILE_TAIL, ({ "Prehensile Tail" }))\
macro(GIFT_QUILLS, ({ "Quills" }))\
macro(GIFT_SPRAY, ({ "Spray" }))\
macro(GIFT_VENEMOUS_BITE, ({ "Venemous Bite" }))\
macro(GIFT_ALLY, ({ "Ally" }))\
macro(GIFT_GANG_OF_IRREGULARS,\
({\
    "Gang of Irregulars",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_ALLY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_IMPROVED_ALLY,\
({\
    "Improved Ally",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_ALLY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    DESCRIPTOR_MULTIPLE, 1\
}))\
macro(GIFT_AMBIDEXTERITY, ({ "Ambidexterity" }))\
macro(GIFT_AKIMBO_FIGHTER,\
({\
    "Akimbo Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_AMBIDEXTERITY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_TANDEM_REPLAY,\
({\
    "Tandem Replay",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_AMBIDEXTERITY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_TANDEM_STRIKE,\
({\
    "Tandem Strike",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_AMBIDEXTERITY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_COUNTER_TACTICS, ({ "Counter-Tactics" }))\
macro(GIFT_ALL_OUT_ATTACK,\
({\
    "All-Out Attack",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_COUNTER_TACTICS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_GUARD_BREAKER,\
({\
    "Guard Breaker",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_COUNTER_TACTICS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_KNOCKDOWN_STRIKE,\
({\
    "Knockdown Strike",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_COUNTER_TACTICS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MOB_FIGHTER,\
({\
    "Mob Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_COUNTER_TACTICS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_THREATENING_FIGHTER,\
({\
    "Threatening Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_COUNTER_TACTICS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_DANGER_SENSE, ({ "Danger Sense" }))\
macro(GIFT_BODYGUARD,\
({\
    "Bodyguard",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_DANGER_SENSE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_BLIND_FIGHTING,\
({\
    "Blind-Fighting",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_DANGER_SENSE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_PRUDENCE,\
({\
    "Prudence",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_DANGER_SENSE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SIXTH_SENSE,\
({\
    "Sixth Sense",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_DANGER_SENSE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_STITCH_IN_TIME,\
({\
    "Stitch in Time",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_DANGER_SENSE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_FAST_MOVER, ({ "Fast Mover" }))\
macro(GIFT_ARTFUL_DODGER,\
({\
    "Artful Dodger",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FAST_MOVER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MAD_SPRINT,\
({\
    "Mad Sprint",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FAST_MOVER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RAPID_DASH,\
({\
    "Rapid Dash",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FAST_MOVER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RAPID_SPRINT,\
({\
    "Rapid Sprint",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FAST_MOVER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_FENCING, ({ "Fencing" }))\
macro(GIFT_DISARMING_STRIKE,\
({\
    "Disarming Strike",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FENCING,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_FENCING_REPLAY,\
({\
    "Fencing Replay",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FENCING,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RAPIER_LUNGE,\
({\
    "Rapier Lunge",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_FENCING,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_LITERACY, ({ "Literacy" }))\
macro(GIFT_ADMINISTRATION,\
({\
    "Administration",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_ASTROLOGY,\
({\
    "Astrology",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_CARTOGRAPHY,\
({\
    "Cartography",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_DOCTOR,\
({\
    "Doctor",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MATHEMATICS,\
({\
    "Mathematics",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RESEARCH,\
({\
    "Research",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_TRADEWINDS_NAVIGATION,\
({\
    "Tradewinds Navigation",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_QUICK_DRAW, ({ "Quick Draw" }))\
macro(GIFT_QUICK_SHEATHE,\
({\
    "Quick Sheathe",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_QUICK_DRAW,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SECOND_THROW,\
({\
    "Second Throw",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_QUICK_DRAW,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SUDDEN_DRAW,\
({\
    "Sudden Draw",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_QUICK_DRAW,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RESOLVE, ({ "Resolve" }))\
macro(GIFT_ARMORED_FIGHTER,\
({\
    "Armored Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_GUARD_SOAK,\
({\
    "Guard Soak",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RELENTLESSNESS,\
({\
    "Relentlessness",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SCARY_FIGHTER,\
({\
    "Scary Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SHIELD_SOAK,\
({\
    "Shield Soak",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_RESOLVE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SHARPSHOOTER, ({ "Sharpshooter" }))\
macro(GIFT_AIMING_ON_THE_DRAW,\
({\
    "Aiming on the Draw",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_SHARPSHOOTER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_COUNTER_SHOT,\
({\
    "Counter Shot",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_SHARPSHOOTER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_INSTINCTIVE_SHOT,\
({\
    "Instinctive Shot",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_SHARPSHOOTER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SNIPERS_SHOT,\
({\
    "Sniper's Shot",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_SHARPSHOOTER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_STREETWISE, ({ "Streetwise" }))\
macro(GIFT_FORGERY,\
({\
    "Forgery",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STREETWISE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    DESCRIPTOR_MULTIPLE, 1\
}))\
macro(GIFT_SABOTAGE,\
({\
    "Sabotage",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STREETWISE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SNEAKY_FIGHTER,\
({\
    "Sneaky Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STREETWISE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SKULKING,\
({\
    "Skulking",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STREETWISE,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_STRENGTH, ({ "Strength" }))\
macro(GIFT_IMPROVED_STRENGTH,\
({\
    "Improved Strength",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STRENGTH,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_INDOMITABLE_FIGHTER,\
({\
    "Indomitable Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STRENGTH,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_LINE_BREAKER,\
({\
    "Line Breaker",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STRENGTH,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MIGHTY_GRIP,\
({\
    "Mighty Grip",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STRENGTH,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MIGHTY_STRIKE,\
({\
    "Mighty Strike",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_STRENGTH,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_TRUE_LEADER, ({ "True Leader" }))\
macro(GIFT_COMMANDING_LEADER,\
({\
    "Commanding Leader",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_TRUE_LEADER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_MILITIA_LEADER,\
({\
    "Militia Leader",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_TRUE_LEADER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_TROOP_LEADER,\
({\
    "Troop Leader",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_TRUE_LEADER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_WATCHFUL_LEADER,\
({\
    "Watchful Leader",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_TRUE_LEADER,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_VETERAN, ({ "Veteran" }))\
macro(GIFT_BRAVERY,\
({\
    "Bravery",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_FOCUSED_FIGHTER,\
({\
    "Focused Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_KNOCKOUT_STRIKE,\
({\
    "Knockout Strike",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RAPID_AIM,\
({\
    "Rapid Aim",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_RAPID_GUARD,\
({\
    "Rapid Guard",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_SHIELD_FIGHTER,\
({\
    "Shield Fighter",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_VETERAN,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 1\
}))\
macro(GIFT_ANONYMOUS,\
({\
    "Anonymous",\
    (Requirement[])\
    {\
        {\
            .descriptor_flags = DESCRIPTOR_INFLUENCE,\
            .requirement_type = REQUIREMENT_NO_GIFT_WITH_DESCRIPTOR\
        },\
        {\
            .requirement_type = REQUIREMENT_HOST_PERMISSION\
        }\
    },\
    0, 2\
}))\
macro(GIFT_ELEMENTAL_APPRENTICE,\
({\
    "Elemental Apprentice",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        },\
        {\
            .gift_index = GIFT_ELEMENTALISTS_TRAPPINGS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 2\
}))\
macro(GIFT_GREEN_AND_PURPLE_MAGIC_APPRENTICE,\
({\
    "Green & Purple Magic Apprentice",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY, \
            .requirement_type = REQUIREMENT_GIFT\
        },\
        {\
            .gift_index = GIFT_COGNOSCENTES_TRAPPINGS, \
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 2\
}))\
macro(GIFT_THAUMATURGY_APPRENTICE,\
({\
    "Thaumaturgy Apprentice",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        },\
        {\
            .gift_index = GIFT_THAUMATURGES_TRAPPINGS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 2\
}))\
macro(GIFT_WHITE_MAGIC_APPRENTICE,\
({ "White Magic Apprentice",\
    (Requirement[])\
    {\
        {\
            .gift_index = GIFT_LITERACY,\
            .requirement_type = REQUIREMENT_GIFT\
        },\
        {\
            .gift_index = GIFT_CLERICS_TRAPPINGS,\
            .requirement_type = REQUIREMENT_GIFT\
        }\
    },\
    0, 2\
}))

enum GiftIndex
{
    GIFTS(MAKE_ENUM)
};

Gift g_gifts[] = { GIFTS(MAKE_VALUE) };