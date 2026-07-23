using UnrealBuildTool;

public class VibeProjectTest : ModuleRules
{
	public VibeProjectTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore",
			"UMG", "Slate", "SlateCore", "DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP", "Json", "JsonUtilities"
		});
	}
}
