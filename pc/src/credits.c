#include "functions.h"
#include "animover.h"

CreditEntry creditEntries[33];

char *creditManagingDirector = "MANAGING DIRECTOR";
char *creditDirectorOfDevelopment = "DIRECTOR OF DEVELOPMENT";
char *creditStudioManager = "STUDIO MANAGER";
char *creditRegionalHeadProducer = "REGIONAL HEAD PRODUCER";
char *creditHeadProducer = "HEAD PRODUCER";
char *creditProducer = "PRODUCER";
char *creditAssociateProducer = "ASSOCIATE PRODUCER";
char *creditGameDesigner = "GAME DESIGNER";
char *creditGamePlayCoOrdinator = "GAME PLAY CO ORDINATOR";
char *creditPcVersionTeam = "PC VERSION TEAM";
char *creditPcProgrammingTeam = "PROGRAMMING TEAM";
char *creditPcArtists = "ARTISTS";
char *creditPlaystation = "PLAYSTATION";
char *creditDevelopmentTeam = "DEVELOPMENT TEAM";
char *creditPlaystationProgrammingTeam = "PROGRAMMING TEAM";
char *creditPlaystationArtists = "ARTISTS";
char *creditIntroMovieMusic = "INTRO MOVIE MUSIC";
char *creditExtroMovieMusic = "EXTRO MOVIE MUSIC";
char *creditMusic = "MUSIC";
char *creditSoundEffects = "SOUND EFFECTS";
char *creditProductManager = "PRODUCT MANAGER";
char *creditPrManager = "PR MANAGER";
char *creditCreativeServices = "CREATIVE SERVICES";
char *creditTerritoriesMarketing = "TERRITORIES MARKETING";
char *creditTechnicalSupport = "TECHNICAL SUPPORT";
char *creditTechnicalAssistants = "TECHNICAL ASSISTANTS";
char *creditDevelopmentSecretary = "DEVELOPMENT SECRETARY";
char *creditManualWrittenBy = "MANUAL WRITTEN BY";
char *creditCreativeDesigner = "CREATIVE DESIGNER";
char *creditPackagingDesign = "PACKAGING DESIGN";
char *creditQualityAssurance = "QUALITY ASSURANCE";
char *creditQaManager = "QA MANAGER";
char *creditSpecialThanksTo = "SPECIAL THANKS TO";
char *creditIanHetherington = "IAN HETHERINGTON";
char *creditAdrianParr = "ADRIAN PARR";
char *creditSarahHarris = "SARAH HARRIS";
char *creditMorganORahilly = "MORGAN O RAHILLY";
char *creditFeargusCarroll = "FEARGUS CARROLL";
char *creditAndySatterthwaite = "ANDY SATTERTHWAITE";
char *creditRobFrancis = "ROB FRANCIS";
char *creditNickBurcombe = "NICK BURCOMBE";
char *creditPaulAhThion = "PAUL AH-THION";
char *creditStewartSockett = "STEWART SOCKETT";
char *creditNickKimberley = "NICK KIMBERLEY";
char *creditPolSigerson = "POL SIGERSON";
char *creditDaveCrook = "DAVE CROOK";
char *creditJohnHarris = "JOHN HARRIS";
char *creditAshleySanders = "ASHLEY SANDERS";
char *creditCraigHouston = "CRAIG HOUSTON";
char *creditChrisRoberts = "CHRIS ROBERTS";
char *creditNickyCarusWestcott = "NICKY CARUS WESTCOTT";
char *creditEvePenford = "EVE PENFORD";
char *creditDarrenDouglas = "DARREN DOUGLAS";
char *creditJeffBramfitt = "JEFF BRAMFITT";
char *creditMickHarrison = "MICK HARRISON";
char *creditHerdKilling = "HERD KILLING";
char *creditPerformedBy = "PERFORMED BY";
char *creditFutureSoundOfLondon = "FUTURE SOUND OF LONDON";
char *creditSplicedByColdStorage = "SPLICED BY COLD STORAGE";
char *creditXylem = "XYLEM";
char *creditPhloem = "PHLOEM";
char *creditArgon = "ARGON";
char *creditXenon = "XENON";
char *creditExtroPerformedByColdStorage = "PERFORMED BY COLD STORAGE";
char *creditBodyInMotion = "BODY IN MOTION";
char *creditCanada = "CANADA";
char *creditHakapikMurder = "HAKAPIK MURDER";
char *creditPlasticity = "PLASTICITY";
char *creditMessijReceived = "MESSIJ RECEIVED";
char *creditSurgeon = "SURGEON";
char *creditTenation = "TENATION";
char *creditMessijXtnd = "MESSIJ XTND";
char *creditKinkong = "KINKONG";
char *creditMusicPerformedByColdStorage = "PERFORMED BY COLD STORAGE";
char *creditTimWright = "TIM WRIGHT";
char *creditSueCampbell = "SUE CAMPBELL";
char *creditGlenOconnell = "GLEN OCONNELL";
char *creditLornaOwen = "LORNA OWEN";
char *creditKellyEvans = "KELLY EVANS";
char *creditSabineWestenberger = "SABINE WESTENBERGER";
char *creditIngoZabrowoski = "INGO ZABROWOSKI";
char *creditCarolineDupuy = "CAROLINE DUPUY";
char *creditBenoiteLavie = "BENOITE LAVIE";
char *creditDawnWilliams = "DAWN WILLIAMS";
char *creditJaneHickey = "JANE HICKEY";
char *creditCarolynSeager = "CAROLYN SEAGER";
char *creditMicheleHarris = "MICHELE HARRIS";
char *creditMarkDay = "MARK DAY";
char *creditStefanieWassell = "STEFANIE WASSELL";
char *creditSteveHenstead = "STEVE HENSTEAD";
char *creditCraigDuddle = "CRAIG DUDDLE";
char *creditChrisStanley = "CHRIS STANLEY";
char *creditJenniferRees = "JENNIFER REES";
char *creditDamonFairclough = "DAMON FAIRCLOUGH";
char *creditAnthonyRoberts = "ANTHONY ROBERTS";
char *creditJimBowers = "JIM BOWERS";
char *creditPaulTweedle = "PAUL TWEEDLE";
char *creditTonyCross = "TONY CROSS";
char *creditMarkInman = "MARK INMAN";
char *creditDaveParkinson = "DAVE PARKINSON";
char *creditRichardYandle = "RICHARD YANDLE";
char *creditDaveBlakeborough = "DAVE BLAKEBOROUGH";
char *creditAndyDurney = "ANDY DURNEY";
char *creditStephenAllen = "STEPHEN ALLEN";
char *creditThomasRees = "THOMAS REES";
char *creditJeffBird = "JEFF BIRD";
char *creditRobSutton = "ROB SUTTON";
char *creditKevinTurner = "KEVIN TURNER";
char *creditDaveRose = "DAVE ROSE";
char *creditDominicMallinson = "DOMINIC MALLINSON";
char *creditAndyCrowley = "ANDY CROWLEY";
char *creditPeteSheppard = "PETE SHEPPARD";

/* Fills the credit table: a title and its name lines per heading. */
void InitCredits(void)
{
    int i = 0;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditManagingDirector;
    creditEntries[i++].lines[0] = creditIanHetherington;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditDirectorOfDevelopment;
    creditEntries[i++].lines[0] = creditAdrianParr;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditStudioManager;
    creditEntries[i++].lines[0] = creditSarahHarris;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditRegionalHeadProducer;
    creditEntries[i++].lines[0] = creditMorganORahilly;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditHeadProducer;
    creditEntries[i++].lines[0] = creditFeargusCarroll;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditProducer;
    creditEntries[i++].lines[0] = creditAndySatterthwaite;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditAssociateProducer;
    creditEntries[i++].lines[0] = creditRobFrancis;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditGameDesigner;
    creditEntries[i++].lines[0] = creditNickBurcombe;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditGamePlayCoOrdinator;
    creditEntries[i++].lines[0] = creditRobFrancis;

    creditEntries[i].count = 0;
    creditEntries[i++].title = creditPcVersionTeam;

    creditEntries[i].count = 3;
    creditEntries[i].title = creditPcProgrammingTeam;
    creditEntries[i].lines[0] = creditPaulAhThion;
    creditEntries[i].lines[1] = creditStewartSockett;
    creditEntries[i++].lines[2] = creditNickKimberley;

    creditEntries[i].count = 4;
    creditEntries[i].title = creditPcArtists;
    creditEntries[i].lines[0] = creditAshleySanders;
    creditEntries[i].lines[1] = creditDaveCrook;
    creditEntries[i].lines[2] = creditJohnHarris;
    creditEntries[i++].lines[3] = creditCraigHouston;

    creditEntries[i].count = 0;
    creditEntries[i++].title = creditPlaystation;

    creditEntries[i].count = 0;
    creditEntries[i++].title = creditDevelopmentTeam;

    creditEntries[i].count = 3;
    creditEntries[i].title = creditPlaystationProgrammingTeam;
    creditEntries[i].lines[0] = creditStewartSockett;
    creditEntries[i].lines[1] = creditChrisRoberts;
    creditEntries[i++].lines[2] = creditNickKimberley;

    creditEntries[i].count = 9;
    creditEntries[i].title = creditPlaystationArtists;
    creditEntries[i].lines[0] = creditNickyCarusWestcott;
    creditEntries[i].lines[1] = creditPolSigerson;
    creditEntries[i].lines[2] = creditDaveCrook;
    creditEntries[i].lines[3] = creditJohnHarris;
    creditEntries[i].lines[4] = creditEvePenford;
    creditEntries[i].lines[5] = creditDarrenDouglas;
    creditEntries[i].lines[6] = creditJeffBramfitt;
    creditEntries[i].lines[7] = creditAshleySanders;
    creditEntries[i++].lines[8] = creditMickHarrison;

    creditEntries[i].count = 4;
    creditEntries[i].title = creditIntroMovieMusic;
    creditEntries[i].lines[0] = creditHerdKilling;
    creditEntries[i].lines[1] = creditPerformedBy;
    creditEntries[i].lines[2] = creditFutureSoundOfLondon;
    creditEntries[i++].lines[3] = creditSplicedByColdStorage;

    creditEntries[i].count = 5;
    creditEntries[i].title = creditExtroMovieMusic;
    creditEntries[i].lines[0] = creditXylem;
    creditEntries[i].lines[1] = creditPhloem;
    creditEntries[i].lines[2] = creditArgon;
    creditEntries[i].lines[3] = creditXenon;
    creditEntries[i++].lines[4] = creditExtroPerformedByColdStorage;

    creditEntries[i].count = 9;
    creditEntries[i].title = creditMusic;
    creditEntries[i].lines[0] = creditBodyInMotion;
    creditEntries[i].lines[1] = creditHakapikMurder;
    creditEntries[i].lines[2] = creditPlasticity;
    creditEntries[i].lines[3] = creditMessijReceived;
    creditEntries[i].lines[4] = creditSurgeon;
    creditEntries[i].lines[5] = creditTenation;
    creditEntries[i].lines[6] = creditMessijXtnd;
    creditEntries[i].lines[7] = creditKinkong;
    creditEntries[i++].lines[8] = creditMusicPerformedByColdStorage;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditSoundEffects;
    creditEntries[i++].lines[0] = creditTimWright;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditProductManager;
    creditEntries[i++].lines[0] = creditSueCampbell;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditPrManager;
    creditEntries[i++].lines[0] = creditGlenOconnell;

    creditEntries[i].count = 2;
    creditEntries[i].title = creditCreativeServices;
    creditEntries[i].lines[0] = creditLornaOwen;
    creditEntries[i++].lines[1] = creditKellyEvans;

    creditEntries[i].count = 10;
    creditEntries[i].title = creditTerritoriesMarketing;
    creditEntries[i].lines[0] = creditSabineWestenberger;
    creditEntries[i].lines[1] = creditIngoZabrowoski;
    creditEntries[i].lines[2] = creditCarolineDupuy;
    creditEntries[i].lines[3] = creditBenoiteLavie;
    creditEntries[i].lines[4] = creditDawnWilliams;
    creditEntries[i].lines[5] = creditJaneHickey;
    creditEntries[i].lines[6] = creditCarolynSeager;
    creditEntries[i].lines[7] = creditMicheleHarris;
    creditEntries[i].lines[8] = creditMarkDay;
    creditEntries[i++].lines[9] = creditStefanieWassell;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditTechnicalSupport;
    creditEntries[i++].lines[0] = creditSteveHenstead;

    creditEntries[i].count = 2;
    creditEntries[i].title = creditTechnicalAssistants;
    creditEntries[i].lines[0] = creditCraigDuddle;
    creditEntries[i++].lines[1] = creditChrisStanley;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditDevelopmentSecretary;
    creditEntries[i++].lines[0] = creditJenniferRees;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditManualWrittenBy;
    creditEntries[i++].lines[0] = creditDamonFairclough;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditCreativeDesigner;
    creditEntries[i++].lines[0] = creditAnthonyRoberts;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditPackagingDesign;
    creditEntries[i++].lines[0] = creditJimBowers;

    creditEntries[i].count = 11;
    creditEntries[i].title = creditQualityAssurance;
    creditEntries[i].lines[0] = creditPaulTweedle;
    creditEntries[i].lines[1] = creditTonyCross;
    creditEntries[i].lines[2] = creditMarkInman;
    creditEntries[i].lines[3] = creditDaveParkinson;
    creditEntries[i].lines[4] = creditRichardYandle;
    creditEntries[i].lines[5] = creditDaveBlakeborough;
    creditEntries[i].lines[6] = creditAndyDurney;
    creditEntries[i].lines[7] = creditStephenAllen;
    creditEntries[i].lines[8] = creditThomasRees;
    creditEntries[i].lines[9] = creditJeffBird;
    creditEntries[i++].lines[10] = creditRobSutton;

    creditEntries[i].count = 1;
    creditEntries[i].title = creditQaManager;
    creditEntries[i++].lines[0] = creditKevinTurner;

    creditEntries[i].count = 4;
    creditEntries[i].title = creditSpecialThanksTo;
    creditEntries[i].lines[0] = creditDaveRose;
    creditEntries[i].lines[1] = creditDominicMallinson;
    creditEntries[i].lines[2] = creditAndyCrowley;
    creditEntries[i++].lines[3] = creditPeteSheppard;
}

/* Draws the scrolling credits; y is the scroll position. */
void ShowCredits(int y)
{
    int group;
    int index;

    y -= 3000;
    for (group = 0; group < 33; group++) {
        if (y > -10 && y < 264) {
            CentreText(creditEntries[group].title, 230, y, 8, 1);
        }
        if (creditEntries[group].count) {
            y += 16;
            for (index = 0; index < creditEntries[group].count; index++) {
                if (y > -10 && y < 264) {
                    CentreText(creditEntries[group].lines[index], 230, y, 8, 0);
                }
                y += 12;
            }
        }
        y += 32;
    }
}
