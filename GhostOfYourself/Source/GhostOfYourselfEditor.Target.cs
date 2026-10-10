using UnrealBuildTool;

public class GhostOfYourselfEditorTarget : TargetRules
{
    public GhostOfYourselfEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("GhostOfYourself");
    }
}
