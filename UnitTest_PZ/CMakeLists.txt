# @file neopz/UnitTest_PZ/CMakeList.txt  -- First Cmake file for Unit Testing modules

if(BUILD_UNITTESTING)
	ENABLE_TESTING()

	set(RUNTIME_OUTPUT_DIRECTORY pz PARENT_SCOPE)

	if(USING_BOOST)

		add_subdirectory(TestFad)
		add_subdirectory(TestTopology)
		add_subdirectory(TestMatrix)
		add_subdirectory(TestMesh)
		add_subdirectory(TestMaterial)
		add_subdirectory(TestIntegNum)
		add_subdirectory(TestGeometry)

		source_group(UnitTestFad FILES ${pztestfad})
		source_group(UnitTestTopology FILES ${pztesttopology})
		source_group(UnitTestMatrix FILES ${pztestmatrix})
		source_group(UnitTestMesh FILES ${pztestmesh})
		source_group(UnitTestIntegNum FILES ${pztestintegnum})
		source_group(UnitTestGeometry FILES ${pztestgeometry})

	else()

		set(pztest ${pztest} ${pztestfad} ${pztesttopology} ${pztestmatrix} ${pztestmesh} ${pztestintegnum})

	endif()
endif()

