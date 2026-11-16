#ifndef PRISONER_RANSOM_H
#define PRISONER_RANSOM_H

#include "Types.h"
#include "Campaign Types.h"

// travnik: расчет денежного выкупа за освобожденных военнопленных
class CPrisonerRansom
{
public:
	// travnik: сумма выкупа за военнопленного указанного типа с учетом прогресса игры
	static UINT32 GetRansomMoney(UINT8 ubPrisonerType);

	// travnik: сумма выкупа для военнопленного, обрабатываемого по счетчикам уже допрошенных
	static UINT32 GetRansomMoneyByProcessedIndex(UINT32 uiProcessedIndex, const UINT8* pubInterrogatedPrisoners);

private:
	// travnik: базовая сумма выкупа из настроек, без учета прогресса игры
	static UINT32 GetBaseRansomMoney(UINT8 ubPrisonerType);

	// travnik: множитель выкупа от текущего прогресса игры
	static FLOAT GetProgressMultiplier();

	// travnik: определяет тип военнопленного по порядку обработки
	static UINT8 GetPrisonerTypeByProcessedIndex(UINT32 uiProcessedIndex, const UINT8* pubInterrogatedPrisoners);
};

#endif
