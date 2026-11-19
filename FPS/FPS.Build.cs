// Copyright TemmieTim

using UnrealBuildTool;

public class FPS : ModuleRules
{
	public FPS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"PhysicsCore",
			"UMG", "Slate", "SlateCore",
			"AIModule"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {   });
	}
}
