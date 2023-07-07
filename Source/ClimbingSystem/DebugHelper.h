#pragma once

namespace Debug
{
    void static Print(const FString &Msg, const FColor &Color = FColor::MakeRandomColor(), int32 InKey = -1)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(InKey, 6.f, Color, Msg);
        }

        UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
    }
}