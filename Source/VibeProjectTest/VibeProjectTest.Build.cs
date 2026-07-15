using UnrealBuildTool;

public class VibeProjectTest : ModuleRules
{
	public VibeProjectTest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core", "CoreUObject", "Engine", "InputCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP", "Json", "JsonUtilities"
		});
	}
}
