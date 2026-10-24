#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* Prototypes for every game function, grouped by source file. */

#include "wipeout.h"
#include "pc_config.h"
#include "pc_sound.h"
#include "pc_ddraw.h"
#include "pc_image.h"
#include "ajs.h"
#include "scene.h"
#include "particle_palette.h"
#ifdef NON_MATCHING
#include "pc_projection.h"
#include "pc_envmap.h"
#include "pc_wave.h"
#include "pc_sfx.h"
#include "pc_sfxds.h"
#include "pc_billboard.h"
#include "pc_mesh.h"
#endif
#include "ships.h"
#include "control.h"
#include "pad.h"

typedef struct CdlPos CdlPos;
typedef struct CheckPoints CheckPoints;
typedef struct DisplayTexPrims DisplayTexPrims;
typedef struct HiScoreData HiScoreData;
typedef struct MenuBox MenuBox;
typedef struct MenuPrims MenuPrims;
typedef struct MonoRailData MonoRailData;
typedef struct RaceData RaceData;
typedef struct ScoreData ScoreData;
typedef struct ResetData ResetData;
typedef struct SParticle SParticle;
typedef struct TexTemplate TexTemplate;
typedef struct Trail Trail;
typedef struct WeaponData WeaponData;
typedef struct WeaponGrid WeaponGrid;
typedef struct WeaponShape WeaponShape;
struct WeaponGridZone;
typedef struct WipeCdStatus WipeCdStatus;
typedef struct SpecialGrid SpecialGrid;
typedef struct AutoMeshData AutoMeshData;
typedef struct Checkpoint Checkpoint;
typedef struct DroidData DroidData;
typedef struct Shadow Shadow;
#include "menus.h"
struct Texture;
struct ConfigData;
struct ScoreData;
struct Speedo;
#include "iso9660.h"
#include "malloc.h"
#include "object2.h"
struct SelectBox;
#include "tim.h"
struct InGameMenu;

/* main.c */
int main(void);

/* object.c */
void InitMatrix(MATRIX *m);
void InitialiseAmbientColour(void);
void InitialiseColourMatrix(void);
void InitialiseLightMatrix(void);
void MatrixMultiply(MATRIX *m1, MATRIX *m2, MATRIX *m3);
void SetCameraDirection(short x, short y, short z);
void SetCameraPosition(long x, long y, long z);
void SetupLights(Object *lights);
int DotProduct(VECTOR *a, VECTOR *b);
void DrawEnvMap(Object *object, SVECTOR *normals, int x, int y, float scale);
void InitFonts(void);
void MyCrossProduct(VECTOR *a, VECTOR *b);
void Normalise(VECTOR *vector);
void PreCalcNormals(Object *object, SVECTOR *normals);

/* malloc.c */
void *DAlloc(HeapBlock *head, long size);
extern HeapBlock *heap;
void DFree(HeapBlock *head, char *ptr);
void InitDynamicMem(HeapBlock *head, long size);

/* file.c */
long FileLength(char *name);
void FileAccessError(char *name);
long LoadFile(char *name, char *buffer);
void SaveFile(char *name, char *buffer, long length);
void InitCDFS(HeapBlock *heap);

/* tim.c */
Texture *LoadTexture(char *name, short translucent);
Texture *WADLoadTexture(WADFILE *wad, short translucent);
Texture *loadtexcom(Tim *tim, short translucent);

/* skeleton.c */
MATRIX * GetSkeletonMatrix(Skeleton *skeleton);
Skeleton * NewSkeleton(void);
void SetSkeletonDirection(Skeleton *skeleton, short xa, short ya, short za);
void SetSkeletonDirectionHPR(Skeleton *skeleton, short hdg, short pitch, short roll);
void SetSkeletonDirectionHRP(Skeleton *skeleton, short hdg, short roll, short pitch);
void SetSkeletonDirectionRPH(Skeleton *skeleton, short roll, short pitch, short hdg);
void SetSkeletonPosition(Skeleton *skeleton, long x, long y, long z);
void SetSkeletonTransform(Skeleton *skeleton);

/* track.c */
void CheckCracks(void);
void CheckVersion(void);
void DrawAllTrack(TrackCamera *camera);
void DrawNewTrack(TrackCamera *camera);
void IndexToPointers(void);
void IntelFace(void);
void IntelSection(void);
void IntelVertex(void);
TrackSection * NextIndexSection(TrackSection *section);
void PointersToIndex(void);
TrackSection * PrevIndexSection(TrackSection *section);
void WADInitFaces(WADFILE *wad);
void WADInitSections(WADFILE *wad);
void WADInitVertices(WADFILE *wad);
void WADInitViewList(WADFILE *wad);

/* draw.c */
void DrawLandscape(ShipData *playerShip, TrackCamera *camera);
char TrackDirection(TrackCamera *camera);

/* ttf.c */
TexTemplate * LoadTtfFile(char *file, short *count, short libraryTextures);
TexTemplate * WADLoadTtfFile(WADFILE *wad, short *count);

/* camera.c */
void InitCamera(TrackCamera *camera, TrackSection *section);
void SetUnitFinVector(VECTOR *unitFinVector, ShipData *camShip);
void UpdateCameraAttractCircle(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraAttractDestroyed(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraAttractInternal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraAttractMedal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraAttractMode(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraDroidView(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraRaceExternal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraRaceInternal(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraRaceStart(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);
void UpdateCameraStaticFollow(TrackCamera *camera, ShipData *camShip, RescueData *rescueDroid);

/* ships.c */
void CopySharkVerts(Object *ship);
void DrawShips(ShipData *shipIndex, Object **shipShapes, int camShip, TrackCamera camera, int flg);
void InitTransShip(Object *lights, short flag);
void SetSpread(ShipData *shipIndex, ConfigData *cfg, short pos, short num, short bigGrid);
void UpdateSharkModel(void);
void initShipData(TrackSection *section, ShipData *shipIndex, ConfigData *options);
void loadShipData(TrackSection *section, ShipData *shipIndex, Object **shipShapes, Skeleton *camPos);

/* control.c */
void GeneralStartProcs(ShipData *s);
void SetUpFirstCheckPoint(void);
void SortAllShips(ShipData *shipIndex);
void UpdatePlayerShip(ShipData *ships, void *shapes, short shipNo, void *weapons);
void UpdatePlayerShipStart(ShipData *shipIndex, void *shipShapes, short shipNo, void *weaponIndex);
void UpdateShips(ShipData *shipIndex, WeaponShape *unused, Object **shipShapes, WeaponData *weaponIndex);

/* utils.c */
int abs(int value);
void CrossProduct(VECTOR *out, VECTOR a, VECTOR b);
int GetAng(VECTOR vec1, VECTOR vec2);
int GetMagnitude(float x, float y, float z);
void MinusVector(VECTOR v1, VECTOR v2, VECTOR *result);
int ScalarProduct(VECTOR v, VECTOR w);
int ang(int angle);
int arccos(int value);
void assign(VECTOR *out, VECTOR v);
int newRand(void);
#ifdef NETPLAY
void SetLocalRandom(u_char enable);
#endif
int sar(int a, int n);
int scalar_product(const VECTOR *a, const VECTOR *b);

/* search.c */
TrackSection * FindNearestSection(TrackSection *trk, VECTOR pos);
int SectionSearch(ShipData *playerShip, short shipNo);

/* scene.c */
void IntelLong(long *lvalue);
short IntelPrim(Prm prm);
void IntelShort(short *svalue);
Object * LoadPrmCommon(WADFILE *wad, char *name, short textureStart);
Object * LoadPrm(char *name, short textureStart);
Object * NewInstance(Object *source);
Object * WADLoadPrm(WADFILE *wad, short textureStart);

/* weapon.c */
#ifdef NETPLAY
void FireWeapon(ShipData *ship, WeaponData *weapons, int owner, signed char type);
#else
void FireWeapon(ShipData *ship, WeaponData *weapons, int owner, u_char type);
#endif
void InsertWeapon(ShipData *ship, WeaponData *weapons, int owner, char type);
void UpdateWeapons(ShipData *ships, WeaponShape *shapes, WeaponData *weapons);
void DrawWeapons(ShipData *ships, WeaponShape *shapes, WeaponData *weapons);
void CheckPitShips(void);
#ifdef NETPLAY
void DamageCollide(int damageType, int shipNo, int attacker);
#else
void DamageCollide(int damageType, int shipNo);
#endif
void FlipShip(ShipData *playerShip);
void GetCheatWeapon(ShipData *ships, int i);
void UpdateFadeScreen(void);
void UpdatePlasma(ShipData *shipIndex, WeaponShape *weaponShapes, WeaponData *weapon, WeaponData *weaponIndex);
int randWep(void);
void BumpDown(TrackSection *bump);
void BumpFlatten(TrackSection *bump);
void BumpUp(TrackSection *bump);
void ClearWeaponTable(void);
void CycleWeaponGrids(ShipData *ships, struct WeaponGridZone *grids, int noGrids);
void FlattenTrack(void);
void GetFireVector(VECTOR *result, VECTOR target, Face *facePtr, VECTOR shipPos);
void GetNewDroneWeapon(ShipData *ships, int shipNo);
void GetNewWeapon(ShipData *ships, int shipNo);
void InitElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitMinigun(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitPlasma(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitQuake(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitRocket(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitRocketLeft(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitRocketRight(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void InitialiseBumps(void);
void KillMines(WeaponData *weapon, WeaponData *weapons, WeaponShape *shapes);
void MissileLock(short shipNo, ShipData *ships);
void RearMissileLock(short shipNo, ShipData *ships);
void TransformTransparentObject(Object *object, void *camera, int shieldID);
void UpdateElecBolt(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void UpdateMine(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void UpdateMinigun(ShipData *ships, WeaponShape *mineData, WeaponData *weapon, WeaponData *objects);
void UpdateMissile(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void UpdateQuake(ShipData *ships, WeaponShape *unused, WeaponData *weapon, WeaponData *weapons);
void UpdateRocket(ShipData *shipIndex, WeaponShape *weaponShapes, WeaponData *weapon, WeaponData *weaponIndex);
void UpdateShield(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void UpdateSmartBomb(ShipData *ships, WeaponShape *shapes, WeaponData *weapon, WeaponData *weapons);
void Weapon2Mine(WeaponData *weapon, WeaponData *weaponIndex, WeaponShape *weaponShapes);
void initWeapons();

/* dynam.c */
int SetTrackAttributes(TrackSection *section, short sectionNo, WeaponGrid *weaponGrid, int noWeaponGrids, short *specialCount, SpecialGrid *specialGrid);
void SetUnitVectors(ShipData *shipIndex);
void GetTargetPos(VECTOR *result, VECTOR pos, Vector cent1, Vector cent2);
void ShipTrkReaction(ShipData *playerShip, Object **shipShapes, short shipNo);
void getWorldPoint(VECTOR *output, Object *object, Object *objectCollide, int pointNo);

/* collide.c */
int AngleOfCollision(SVector normal, VECTOR ship_point);
void BeamCollideReaction(ShipData *playerShip);
int CheckPolygonCollision(Face *facePtr, VECTOR pos, int alpha, int side);
void DroneShip2TrackBase(ShipData *playerShip, Face *facePtr, int alpha);
int NewCollideLineToPlane(Object *object, Object **shipShapes, ShipData *shipIndex, int shipNo);
#ifdef NETPLAY
void NewWingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo);
#else
void NewWingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha);
#endif
#ifdef NETPLAY
void NoseCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo);
#else
void NoseCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha);
#endif
int PlaneLineCollision(VECTOR plane_point, VECTOR ship_pos, SVector normal);
void Reflection(VECTOR *incidence, SVector normal);
void Ship2ShipCollision(ShipData *shipIndex, Object **shipShapes, int i, int j);
#ifdef NETPLAY
void Ship2Track(ShipData *playerShip, Face *facePtr, Object **shipShapes, short shipNo);
#else
void Ship2Track(ShipData *playerShip, Face *facePtr, Object **shipShapes);
#endif
void Ship2TrackBase(ShipData *playerShip, Face *facePtr, int alpha);
#ifdef NETPLAY
void WingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha, short shipNo);
#else
void WingCollision(ShipData *playerShip, Face *facePtr, int direction, int alpha);
#endif
int collideLineToPlane(int shipNo1, int shipNo2, Object **shipShapes, ShipData *shipIndex);
void doShipShipCollision(int shipNo, ShipData *shipIndex, Object **shipShapes);

/* hud.c */
void AddDelEnd(unsigned char isDelete, long x, long y, long height, long colour);
void AddJustifyNumber(long number, long xInput, long y, long size, long colour);
void AddNumber(int number, int x, int y, int size, int colour);
void AddText(unsigned char *textPointer, long x, long y, long size, long colour);
#ifdef NETPLAY
void AddTime(int time, int x, int y, int size, int colour, signed char flags);
#else
void AddTime(int time, int x, int y, int size, int colour, unsigned char flags);
#endif
void AssignMedals(ConfigData *config);
void BackPoly(POLY_F4 *selectBox, DR_MODE *selectTrans, short x, short y, short w, short h);
short CalcPosition(ShipData *ships);
void CentreText(unsigned char *text, long unusedX, long yInput, long sizeInput, long colour);
void CentreText2(unsigned char *text, long x, long y, long size, long colour);
void DisplayRaceInfo(ConfigData *state, struct ScoreData *context, ShipData *scene);
void EndChallenge(ConfigData *config);
void InitScreenTex(Texture **textures, char *challengeName);
void InitSpeedo(struct Speedo *speedo);
void InitText(void);
void SetCharTextures(unsigned char size);
void UpdateRaceHud(ShipData *shipIndex, int choice, RaceConfig *gameData, HiScoreData *scoreData);
void copyInTempLapTimes(ShipData *ships);
void fill_speedo(ShipData *ships, ConfigData *game, int player);
void AlterCheckPoint(CheckPoints *checkpoints, ShipData *shipIndex, int targetShip, ConfigData *gameData);
void DisplayContinues(short x, short y);

/* raceinit.c */
void GetJoyPadPacketOnVsync(int packetIndex);
void InitAttractMode(int mode, ShipData *ships);
void InitCDAudio(ConfigData *menu, unsigned char *unused);
void InitCheckPoints(CheckPoints *checkpoints, ConfigData *config);
void InitInGameMenu(struct InGameMenu *menu);
void InitRaceData(RaceData *race);
void InitRaceLine(TrackData *track, ConfigData *game);
void InitRaceResetData(struct ResetData *reset, ShipData *ships, ConfigData *config, WeaponData *weapons, WeaponShape *weaponShapes, RescueData *rescue, Object **shipShapes, CheckPoints *checkPoints);
void InitShufflePlay(void);
void InitTrackTextures(ConfigData *config);
void InitWeaponGrids(WeaponGrid *weaponGrid, TrackData *context, int *noWeaponGrids, short *specialCount, SpecialGrid *specialGrid);
int myrand(void);

/* ayrace.c */
struct PauseMenu;
void Check2PlayerPause(u_char status, u_char *lastStatus, int *musicPaused);
int CheckAttractOver(ConfigData *config);
void CheckGamePause(ShipData *ships, PcGameData *config);
void InitiateTwoPlayerPause(struct PauseMenu *pause);
void Perform2PlayerPauseCommand(struct PauseMenu *pause);
void PerformPauseOperations(struct SelectBox *box, PcGameData *config);
void QuitGame(void);
void RaceComplete(void *screen, void *unused, struct ConfigData *gameData, short *choice, struct ScoreData *scoreData, ShipData *ships);
void Restart(ResetData *state);
void SelectPauseOption(int *selection, int canMove, ConfigData *config, struct PauseMenu *pause);
void CheckForButtonPress(void);
void CheckShipDestroyed(ShipData *ships, struct ConfigData *game, int *attract, int idleLimit);
void PlayCredits(int state);
void SetInGameCDTrack(void *unused1, void *unused2, ConfigData *config, int *state);

/* racectrl.c */
#include "racectrl.h"
void InitResident(void);
void ShowLinkBroken(void);
void TrackSpecificFunctions(void);
int race(ConfigData *gameData);
void racetidy(void);

/* init.c */
void GhostLoadCmpFiles(TIMlist *timPtr);
short InitScene(Object **objects, short *objectCount, Skeleton *camera, ConfigData *config);
void LoadCmpFiles(TIMlist *timPtr);
TIMlist * LoadCompressedTextureSequence(char *fileName);
struct Tim;
Texture *NewLoadTexture(struct Tim *tim, short translucent, int scene);
short WADInitScene(Object **objects, short *objectCount, Skeleton *camera, ConfigData *config, WADFILE *wad);
void getPath(char *trackPath, char trackNo, char *name);
TIMlist *WADLoadCompressedTextureSequence(WADFILE *wad);

/* sound.c */
void AdjustCrowdSound(Object **objects, int count);
void AdjustDirectionalNote(short voice, short unused1, short unused2, VECTOR pos);
void AdjustDopplerEngine(int shipNo, short channel, int unused);
void AdjustEngineSound(ShipData *ship, TrackCamera *camera);
void AdjustRemoteEngineSound(int ship1, int ship2, int extra2, int extra3);
int InitCD(CdlLOC *locs, int unused);
int InitCrowdSound(Object **prm, short prmCount, Object **destination, int count);
short PlayDirectionalNote(short sampleId, short pitch, short duration, VECTOR pos);
void InitSpuSamples(void);
void InitSpuSound(void);
int KillControlledNote(short voice);
void LoadSample(int sampleId, char *name);
void PlayControlledNote(int sampleId, int volume, int lrPan, int fbPan, int pitch);
void PlayNote(int sampleId, short volume, int lrPan, int fbPan, int pitch);
void StartControlledSound(void);
void FindVagInVab(void);
void UpdateNotes(int unused);
void KillNote(void);
void KillNotes(void);
void KillFadeNotes(void);
void CdMusicPlay(int track, int unused);
void CdMusicStop(void);
void KillControlledSound(void);
void MuteControlledSound(void);
void TerminateSound(void);

/* options.c */
struct PauseState;
struct OptionsPolys;
void DeSelectScreen(void *screen);
void InGameReset(PcGameData *config, struct SelectBox *box);
void UpdateMusicBar(PcGameData *config);
void UpdateSfxBar(PcGameData *config);
char *GetCdTrackArtist(int track);
char *GetCdTrackName(int track);

/* radscn.c */
void CalcNewRadius(TrackSection *section, Object *objects);
void RadiusControl(TrackCamera *camera, Object **prm);
void SceneRadiusCheck(Object *objects);

/* error.c */
void Error(char *string, short errorLevel);

/* effects.c */
void AddEffects(WeaponData *weapons, ShipData *ships);
void AnimateSprite(Object **objects, short count, short speed, int first, short frames);
void AnimateTexture(Object **objects, short count, short period, short firstTexture, short textureCount, CVECTOR colour, int *frame, int *textureCounter, short textureStep);
void ColourDroidLights(Object *droid);
void ColourRedLight(Object *light);
void ColourStartBoom(int noToLight);
void DoNeonSign(Object **objects, short count);
void DrawShadow(Object **shapes, ShipData *ships);
void DrawWeaponIcons(ShipData *ship);
void InitDroidThings(Object **objects, int unused1, int unused2, Checkpoint *points, ConfigData *options, ShipData *ships);
void InitMonoData(Object **objects, short count, MonoRailData *mono);
void InitObject(Object **prm, short prmCount, Object **found, char *name, short len, short *count);
void InitRedLight(Object **prm, short prmCount, Object **redLight);
Shadow *InitShadow();
void InitShadowCluts(void);
void InitStartBoom(Object **prm, short prmCount);
void InitTrainLights(Object **objects, short objectCount, Object **lights, int mode);
void InitWaterFall(void);
void InitWeaponIcons(POLY_FT4 *icons, struct Texture **textures);
void LoadStartDroid(Object **objects, char *name, Skeleton *skeleton);
void LoadTrain(Object **objects, char *name, Skeleton *parent, VECTOR *positions);
void MoveAdvert(Object **objects, int count);
void MoveCameraDroid(Object **objects, short count, ShipData *ships, int *bobCount);
void MoveFan(Object **objects, int count);
void MoveMonoRail(Object **objects, short count, MonoRailData *mono, ShipData *ships, int mode);
void MoveRadar(Object **objects, short count);
void MoveZeppelin(Object **objects, short objectCount, ShipData *ships);
void RedBullAnimate(Object **objects, short objectCount, short unusedPeriod, short firstTexture, short unusedTextureCount);
void ResetAnimatedObjects(void);
void ResetLightStart(void);
void ResetMonoRail(void);
void ResetStartBoom(void);
void ResetZeppelin(void);
void SetShake(int amount);
void ShakeScreen(void);
void UpdateLightStart(Object **objects, ShipData *ships, CheckCell *cell, short textureBase, ConfigData *config, int mode);
void UpdateStartDroid(Object **shapes, ShipData *ships, DroidData *droids, short textureBase, ConfigData *game, int player);
void Video(Object **objects, int count);
void WaterFall(void);
void InitPitCycle(void);
void InitTrackCycle(void);

/* combat.c */
void _init_cont(void);
void DrawLinkSign(ShipData *ships);
void InitLinkUpSign(void);
void _error(void);
void doSerialGame(int unused, ShipData *ships);
void doSerialMenu(ConfigData *config, u_char *selection);
int doSerialTransfer(u_char *send, u_char *recv, int count);
int receiveSerialPacket(int unused, u_char *data, int len);
int sendSerialPacket(int type, u_char *data, int count);
int _comb_control(int op, int reg, int value);

/* lzss.c */
void ExpandData(unsigned char *inData, unsigned char *outData);

/* iso9660.c */
void LogicalSectorToLoc(u_int sector, CdlLOC *loc);
u_char *NextPath(u_char *dirRecord);
void close_iso_events(void);
int find_file(char *filename, long loc, IsoFileInfo *fi);
int iso_close(int fd);
int iso_get_file_info(char *pathname, IsoFileInfo *fi);
int iso_init(HeapBlock *heap);
long iso_open(char *pathname, long flags, int unusedMode);
long iso_read(void *buffer, long nsec, long fd);
long match_dir(char *dirname, char *testname, u_char test_len);
long match_file(char *filename, char *testname, u_char test_len);
int search(char *filename, IsoFileInfo *fi);
int iso_fileloc(int fileNo);
int iso_filesize(int fileNo);
u_char *iso_toc(void);
void my_error_handler(void);

/* menus.c */
void AudioConfig(ConfigData *gameData);
int CheckCardFull(int card);
void ClassSelect(PcGameData *gameData);
void CopyGameData(ConfigData *gameData, short saveSelection, HiScoreData *score);
u_char DecodePassword(ConfigData *gameData, u_char *password);
#ifdef NETPLAY
void DispHiScores(ConfigData *gameData, char raceMode);
#else
void DispHiScores(ConfigData *gameData, unsigned char raceMode);
#endif
int DisplayCardBack();
void DisplayConfig(ConfigData *gameData, HiScoreData *score);
void DisplayPassword(ConfigData *gameData);
void DoButton(short x, short y, short requestedIcon, char *text);
void DoConfirm(SaveSelection *selection, MenuPolys *polys, short messageType);
void DrawDrBox(int unused, DrBox *box);
void InitDrBox(Texture *texture, DrBox *box);
void DisplayScreen(Texture *texture, DrBox *box);
void DrawHighLight(POLY_F4 (*prim)[2], int x, int y, int width, int height, short lineNum);
void DrawMusicScreen(MusicScreen *menu);
short DupFileCheck(SaveSelection *selection);
short EnterFileName();
char *GetKeyFunction(int index);
char * GetTrackName(short type, short x, short y, short draw);
void InitCard(int card);
void InitDefaultName(ConfigData *gameData, HiScoreData *data, int unused);
void EnterName(ConfigData *gameData, HiScoreData *data);
void InitGameData(ConfigData *gameData);
void InitKeyConfig(KeyConfig *config);
void InitMenuPolys(MenuPrims *polys);
void InitMenuTextures(u_short *textureIndices);
void InitMenus(void);
void InitMusicScreen(MusicScreen *musicScreen, int textureBase);
void InitScreenBox(MenuPolys *screen);
void InitTempData(ConfigData *gameData, MenuSelection *selection);
short IsTeamCardEmpty(int team);
LoadedImage *Load16BitTexture(char *fileName, int loadBothBuffers);
void LoadMenuLineData(void);
void InitLineEditor(void);
int LoadSaveIcons(char *fileName, int iconCount, u_char *iconData, u_char *iconPalette, u_short *iconWidths, u_short *iconHeights, int bufferSize);
void MakePassword(ConfigData *gameData);
void MenuControl(ConfigData *gameData, int timeout);
void MenuFunction(void *gameData, void *polys, void *keyConfig, void *selection, void *cardState, int musicScreen);
/* Menu, HUD text and start-up helpers. Print and PrintCX are in hud.c; ShipParameters, TeamSelect,
   AddTextBits, InitSlider and SlideScreen are in menus.c; the rest are declared only so far. */
void Print(u_char *text, int x, int y, int size);
void PrintCX(char *text, int x, int y, int colour);
void ResetKeyConfig(MenuSelection *selection);
void ResetLoadMenu(SaveSelection *selection);
void ScreenConfig(ConfigData *gameData, MenuPolys *polys, short *screenPosition);
void ShipParameters(PcGameData *gameData, short x, short y);
void ShowMenuBackground(POLY_FT4 *menuPrim, POLY_FT4 *menu1Prim, LoadedImage *image);
void ShowScreen(POLY_FT4 *menuPrim, POLY_FT4 *menu1Prim, LoadedImage *image, int x, int y);
void TeamSelect(PcGameData *gameData);
void AddTextBits(PcGameData *gameData);
void EstablishLink();
void InitChallenges(void);
void InitMovies(void);
void InitRaceLoad(void);
void InitResidentMenu(void);
void InitSlider(void);
void LoadButtons(void);
void ReInitGameProgress(void);
void ShowLoadMessage(void);
void SlideScreen(void);
void killLink(void);

/* trackasm.c */
void AsmGenerateTiles(TexTemplate *tiles, short tileCount);
void AsmHighResTile(TexTemplate *template, short index, short reversed);
void AsmLowResTile(TexTemplate *template, short index, short reversed);
void AsmMediumResTile(TexTemplate *template, short index, short reversed);
void AsmNextFrame(void);

/* card.c */
int CardContentStatus(void);
int CheckCardEvents(void);
int CheckHWCardEvents(void);
void ClearEventHW(void);
void ClearEventSW(void);
int CreateFile(char *fileName);
void FirstSavedGame(void *data);
void GetHeader(u_char *buffer, u_char *name);
void LoadCard(int slot);
u_char LoadCardData(char *name, int slot, ConfigData *settings);
int LoadCardInitData(char *name, short port, ConfigData *settings);
void LoadFirstFile(u_short *count, int slot, char *name);
void MakeFileName(char *suffix, void *header, int slot);
int QuickUnFormat(int slot);
int RenameFile(char *name, int slot, ConfigData *settings);
int RenameFile1(char *card, char *source, int type, void *output);
u_char SaveCardFile(char *name, int slot, ConfigData *settings);
int UnFormat(int slot);
int WriteFile(void *header, char *filename, ConfigData *settings);
void CloseCardEvents(void);
int GetCardInfo(int slot);
void InitCardEvents(void);
void LoadCardTexture(void);

/* crack.c */
void FillHiMedCrack(TrackSection *hiSection, Skeleton *camera);
void FillMedLoCrack(TrackSection *section, Skeleton *view);

/* pad.c */
short GetMenuKey(short a);
void InitController(void);
void InitPadSettings(void);
void SetBrake(ShipData *ship);
void SetDitch(ShipData *ship);
void SetFire(ShipData *ship, WeaponData *weapons, short shipNo);
void SetView(ShipData *ship, ControlInput *control);
void UpdateCheatSequence(int buttonCode);
struct PadPacket;
void buildFixedPad(struct PadPacket *pad);
void buildGameKeyPad(struct PadPacket *pad, ControlInput *control);

/* droid.c */
void InitRescueDroid(RescueData *rescueDroid, Object **shipShapes, ShipData *shipIndex);
void SetDroidVectors(RescueData *d, Object **shapes);
void UpdateDroidCircle(RescueData *d, ShipData *ship, Object **shapes);
void UpdateDroidRescue(RescueData *rescueDroid, ShipData *ship, Object **shapes);
void UpdateDroidStart(RescueData *droid, ShipData *ships, Object **shapes);
void UpdateRescueDroid(ShipData *shipIndex, Object **shipShapes, short shipNo, WeaponData *weaponIndex);

/* animover.c */
void anim(char *fileName);
int LoadProg(int progIndex);

/* credits.c */
void InitCredits(void);
void ShowCredits(int y);

/* display.c */
#include "display.h"
BLK_FILL *BlockFill(short x, short y, short w, short h, short red, short green, short blue);
void CallbackSwap(void);
POLY_F4 *ClearScreen(short red, short green, short blue);
void ClearVram(void);
void InitGameDisplay(int unused);
void InitMenuDisplay(int geomY, int unused);
void InitSonyDisplay(void);
void InitSystem(void);
void Swap(void);

/* progress.c */
void DrawBox(int x, int y, int w, int h, unsigned char r, unsigned char g, unsigned char b);
void Close2DParticle(void);
void DrawLine(int x0, int y0, int x1, int y1, unsigned char r, unsigned char g, unsigned char b);
void Init2DParticle(void);
void InitPop(void);
void Insert2DParticle(int x, int y);
void RefreshCache(void);
void Update2DParticle(void);
void WDclose(void);
int WDopen(int fileNo, int unused1, int unused2);
int WDread(int handle, u_char *destArg, int count);
void VsyncFunc(void);

/* wadfile.c */
void closewadfile(WADFILE *wad);
void closewadfilenvc(WADFILE *wad);
void readnextfile(WADFILE *wad, char *dest);
WADFILE *openwadfile(int fileNo);
WADFILE *openwadfilenvc(int fileNo);

/* sparks.c */
SParticle *InitParticle(void);
void AddRandomParticles(SParticle *particles, int count, VECTOR *pos, VECTOR *vel);
void AddScrapingParticles(SParticle *particles, int count, VECTOR *pos, VECTOR *vel);
void AddSnow(SParticle *particles, int unused, VECTOR *camera);
void AddRain(SParticle *particles, int unused, VECTOR *camera);
void AddExplodingParticles(SParticle *particles, int count, VECTOR *pos);
void AddEboltParticles(SParticle *particles, int count, VECTOR *pos);
void AddRisingParticles(SParticle *particles, int count, VECTOR *pos, int fallSpeed);
void AddFireyParticles(SParticle *particles, int count, VECTOR *pos, int dir, int type, VECTOR *vel);
void AddPitParticles(SParticle *particles, int count, VECTOR *pos, int fallSpeed);
void ClearParticleTable(SParticle *particles);
void LoadExplodeGFX(void);
void InitBirds(void);
void LoadBirdGFX(void);
void InitExplode(void);
void UpdateExplode(void);
void DrawExplode(void);
void UpdateBirds(void);
void DrawBirds(void);
void InitRing(void);
void UpdateRing(void);
void DrawRing(void);
void AddSpurtExplode(int x, int y, int z, int ship);
void AddFireExplode(int x, int y, int z, short ship);
void AddExplode(int x, int y, int z, int ship);
void AddTendrilExplode(int x, int y, int z);
void AddMinigunExplode(int x, int y, int z, int unused);
void AddSmallSpurtExplode(int x, int y, int z, int ship);
void AddSmallSmokeExplode(int x, int y, int z, int ship);
void AddSmokeExplode(int x, int y, int z, int unused);
void AddRing(int x, int y, int z, int unused);
void DrawMissileFlare(int xpos, int ypos, int zpos);
void DrawRocketFlare(int xpos, int ypos, int zpos);
void InitElectric(void);
void AddElectric(VECTOR *position, int mode);
void resetMiscFx(void);
void UnsetParticles(SParticle *particles);

/* trails.c */
void BuildOwnShipTrail(int ship, VECTOR *left, VECTOR *right);
void DrawFlare(int x, int y, int distance, int ship, float z);
void DrawFlashNozzle(int ship, TrackCamera *camera);
void DrawShipTrails(TrackCamera *camera);
void DrawTrail(TrackCamera *camera, Trail *trail, int ship);
void DrawTrailLoRes(TrackCamera *camera, Trail *trail, int ship);
void InitPlasmaShot(void);
void InitTrails(void);
void NewPlasmaSeg(VECTOR *pos, Trail *trail);
void NewPlasmaSegment(VECTOR *pos);
void NewTrailSegment(int ship, VECTOR *left, VECTOR *right);
void UpdateShipTrails(ShipData *ships, int allShips);
void UpdateTarget(int x, int y, int distance, int target);

/* ghost.c */
void InitGhostShip(ConfigData *config, ShipData *ships);
void RecordKeypresses(ShipData *ships);
void buildGhostReplayData(ShipData *ships);
void storeGhostTrail(void *unused, ConfigData *config);

#include "drone.h"

/* sfx.c */
int Big2Little(u_int value);
int GetFreePlayId(void);
int GetFreeVoice(void);
void SetDolbyVolume(int sound);
void SetEffectVolume(int sound);
void SetGlobalVolume(int sound, int cd);
void SetMonoVolume(int sound);
void SetQSoundVolume(int sound);
void SetStereoVolume(int sound);
int SfxChainId(int parent, int sample, int volume, int lrPan, u_char fbPan, int pitch, int flags);
void SfxContinue(void);
void SfxDoSfx(void);
void SfxEffectFBPan(int sound, u_char pan);
void SfxEffectLRPan(int sound, unsigned int pan);
int SfxFreeEffect(unsigned int sample);
void SfxKillAllVoices(void);
void SfxOff(void);
void SfxOn(void);
void SfxPause(void);
void SfxReverbOff(void);
void SfxReverbOn(void);
void SfxSetCallBack(void (*callback)(unsigned int id));
void SfxSetCdVolume(u_char volume);
void SfxSetMode(u_char mode);
void SfxSetVolume(u_char gain);
void SfxStop(int sound);
void SfxUnInit(void);
void SfxUnInitReverb(void);

/* hiscores.c */
void CheckForHiScore(PcGameData *config, HiScoreData *score, ShipData *ships, int shipNum, u8 lap);
void DisplayScoreTab(PcGameData *config, HiScoreData *score);
void EnterHighScore(PcGameData *gameData, HiScoreData *hiScoreData);
void InitScoreTabData(HiScoreData *score);
void ScoreTabInit(PcGameData *gameData);

/* asm */
void AsmDrawObject();
void AsmTrackHi(TrackSection *firstSection, Skeleton *camera, AutoMeshData *autoMesh, short hiCount);
void AsmTrackLo(TrackSection *firstSection, Skeleton *camera, short loCount);
void AsmTrackMed(TrackSection *firstSection, Skeleton *camera, short medCount);
void RotTrans(SVECTOR *v, VECTOR *result, long *flag);

#ifdef NETPLAY
/* Set when a network game is running. */
extern signed char networkGame;
#endif
#ifdef NETPLAY
/* Kills by and against each ship. The NET build reads both with movsx (signed bytes). */
extern signed char killerCount[];
extern signed char killerTotal[];
/* Network race state shared by the race control and the HUD. */
extern int killMessageTimer;
extern int killMessageFlash;
extern int deathCameraActive;
extern int playerDestroyed;
extern int resultsShown;
extern signed char gstSyncActive;
extern signed char livesRemaining[];
#endif

#endif
