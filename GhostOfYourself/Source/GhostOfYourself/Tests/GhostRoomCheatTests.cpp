#include "../GhostReplayController.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGhostRoomCheatTest, "GhostOfYourself.Developer.RoomCheats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGhostRoomCheatTest::RunTest(const FString& Parameters)
{
    AGhostReplayController* Replay = GetMutableDefault<AGhostReplayController>();
    for (int32 Room = 0; Room <= 10; ++Room)
    {
        TestFalse(TEXT("Developer room jumping is removed in every build"), Replay->JumpToRoom(Room));
    }
    return true;
}
#endif
