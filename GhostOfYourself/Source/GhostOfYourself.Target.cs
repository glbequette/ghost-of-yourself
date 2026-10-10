using UnrealBuildTool;

public class GhostOfYourselfTarget : TargetRules
{
    public GhostOfYourselfTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("GhostOfYourself");
    }
}
