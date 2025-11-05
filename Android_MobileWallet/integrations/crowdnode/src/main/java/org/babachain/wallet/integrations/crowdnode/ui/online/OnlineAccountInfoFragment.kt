/*
 * Copyright 2022 Dash Core Group.
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

package org.babachain.wallet.integrations.crowdnode.ui.online

import android.os.Bundle
import android.view.View
import androidx.fragment.app.Fragment
import androidx.navigation.fragment.findNavController
import dagger.hilt.android.AndroidEntryPoint
import org.babachain.wallet.common.ui.viewBinding
import org.babachain.wallet.common.util.safeNavigate
import org.babachain.wallet.integrations.crowdnode.R
import org.babachain.wallet.integrations.crowdnode.databinding.FragmentOnlineAccountInfoBinding

@AndroidEntryPoint
class OnlineAccountInfoFragment : Fragment(R.layout.fragment_online_account_info) {
    private val binding by viewBinding(FragmentOnlineAccountInfoBinding::bind)

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)

        binding.toolbar.setNavigationOnClickListener {
            findNavController().popBackStack()
        }

        binding.createAccountBtn.setOnClickListener {
            safeNavigate(OnlineAccountInfoFragmentDirections.onlineAccountInfoToEmail())
        }
    }
}
