// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HeaderProviderArchiveProxy.h"
#include "UnrealHeaderTool.h"
#include "HeaderProvider.h"
#include "UHTMakefile.h"


FHeaderProvider FHeaderProviderArchiveProxy::CreateHeaderProvider() const
{
	FHeaderProvider Result = FHeaderProvider(static_cast<EHeaderProviderSourceType>(Type), Id, bAutoInclude);
	return Result;
}

void FHeaderProviderArchiveProxy::ResolveHeaderProvider(FHeaderProvider& HeaderProvider, const FUHTMakefile& UHTMakefile) const
{
	HeaderProvider.SetCache(UHTMakefile.GetUnrealSourceFileByIndex(CacheIndex));
}

FArchive& operator<<(FArchive& Ar, FHeaderProviderArchiveProxy& HeaderProviderArchiveProxy)
{
	Ar << HeaderProviderArchiveProxy.Type;
	Ar << HeaderProviderArchiveProxy.Id;
	Ar << HeaderProviderArchiveProxy.CacheIndex;
	Ar << HeaderProviderArchiveProxy.bAutoInclude;

	return Ar;
}
