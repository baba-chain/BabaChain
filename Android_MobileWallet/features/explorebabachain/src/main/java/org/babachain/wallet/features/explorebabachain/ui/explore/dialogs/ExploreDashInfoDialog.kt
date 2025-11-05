/*
 * Copyright 2021 Dash Core Group.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.babachain.wallet.features.exploredash.ui.explore.dialogs

import android.os.Bundle
import android.view.View
import androidx.fragment.app.FragmentActivity
import dagger.hilt.android.AndroidEntryPoint
import org.babachain.wallet.common.services.analytics.AnalyticsService
import org.babachain.wallet.common.ui.dialogs.OffsetDialogFragment
import org.babachain.wallet.common.ui.viewBinding
import org.babachain.wallet.features.exploredash.R
import org.babachain.wallet.features.exploredash.databinding.ExploreDashMainInfoBinding
import javax.inject.Inject

@AndroidEntryPoint
class ExploreDashInfoDialog : OffsetDialogFragment(R.layout.explore_dash_main_info) {

    private val binding by viewBinding(ExploreDashMainInfoBinding::bind)
    @Inject lateinit var analyticsService: AnalyticsService

    private var onCompletion: (() -> Unit)? = null

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.learnMoreLabel.setOnClickListener {
            OffsetDialogFragment(R.layout.buy_gift_card_description).show(requireActivity())
        }
        binding.exploreDashInfoContinueBtn.setOnClickListener {
            dismissAllowingStateLoss()
            onCompletion?.invoke()
        }
    }

    fun show(activity: FragmentActivity, onCompletion: (() -> Unit)? = null) {
        this.onCompletion = onCompletion
        super.show(activity)
    }
}
