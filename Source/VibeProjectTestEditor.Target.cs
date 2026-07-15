using UnrealBuildTool;
using System.Collections.Generic;

public class VibeProjectTestEditorTarget : TargetRules
{
	public VibeProjectTestEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("VibeProjectTest");
	}
}
