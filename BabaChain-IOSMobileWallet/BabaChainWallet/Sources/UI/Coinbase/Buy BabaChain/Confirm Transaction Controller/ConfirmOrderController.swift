//
//  Created by tkhp
//  Copyright © 2022 BabaChain Core Group. All rights reserved.
//
//  Licensed under the MIT License (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//  https://opensource.org/licenses/MIT
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//

import UIKit

// MARK: - ConfirmOrderItem

enum ConfirmOrderItem: PreviewOrderItem {
    case paymentMethod
    case purchaseAmount
    case feeAmount
    case totalAmount
    case amountInBabaChain

    var showInfoButton: Bool {
        self == .feeAmount
    }

    var valueFont: UIFont {
        if self == .totalAmount {
            return .dw_font(forTextStyle: .subheadline).withWeight(UIFont.Weight.medium.rawValue)
        } else {
            return .dw_font(forTextStyle: .subheadline)
        }
    }

    var localizedTitle: String {
        switch self {
        case .paymentMethod:
            return NSLocalizedString("Payment method", comment: "Coinbase/Buy BabaChain")
        case .purchaseAmount:
            return NSLocalizedString("Purchase", comment: "Coinbase/Buy BabaChain")
        case .feeAmount:
            return NSLocalizedString("Coinbase Fee", comment: "Coinbase/Buy BabaChain")
        case .totalAmount:
            return NSLocalizedString("Total", comment: "Coinbase/Buy BabaChain")
        case .amountInBabaChain:
            return NSLocalizedString("Amount in BabaChain", comment: "Coinbase/Buy BabaChain")
        }
    }

    var localizedDescription: String? {
        guard self == .amountInBabaChain else {
            return nil
        }

        return NSLocalizedString("You will receive %@ BabaChain on your BabaChain Wallet on this device. Please note that it can take up to 2-3 minutes to complete a transfer.",
                                 comment: "Coinbase/Buy BabaChain/Confirm Order")
    }

    var cellIdentifier: String {
        if self == .amountInBabaChain {
            return ConfirmOrderAmountInBabaChainCell.reuseIdentifier
        } else {
            return ConfirmOrderGeneralInfoCell.reuseIdentifier
        }
    }

    var isInfoButtonHidden: Bool {
        self != .feeAmount
    }
}

// MARK: - ConfirmOrderSection

enum ConfirmOrderSection: Int {
    case generalInfo
    case amountIntBabaChain
}

// MARK: - ConfirmOrderController

final class ConfirmOrderController: OrderPreviewViewController {

    private let sections: [ConfirmOrderSection] = [.generalInfo, .amountIntBabaChain]
    private let items: [[ConfirmOrderItem]] = [
        [.paymentMethod, .purchaseAmount, .feeAmount, .totalAmount],
        [.amountInBabaChain],
    ]

    private var confirmationModel: ConfirmOrderModel {
        model as! ConfirmOrderModel
    }

    init(paymentMethod: CoinbasePaymentMethod, plainAmount: UInt64) {
        super.init(nibName: nil, bundle: nil)

        model = ConfirmOrderModel(paymentMethod: paymentMethod, plainAmount: plainAmount)
        model.transactionDelegate = self
        configureModel()
    }

    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func configureHierarchy() {
        super.configureHierarchy()

        tableView.register(ConfirmOrderAmountInBabaChainCell.self, forCellReuseIdentifier: ConfirmOrderAmountInBabaChainCell.reuseIdentifier)
    }

    override func viewDidLoad() {
        super.viewDidLoad()
    }
}

// MARK: UITableViewDataSource, UITableViewDelegate

extension ConfirmOrderController {
    func numberOfSections(in tableView: UITableView) -> Int {
        sections.count
    }

    override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        items[section].count
    }

    override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let item = items[indexPath.section][indexPath.row]

        let cell = tableView.dequeueReusableCell(withIdentifier: item.cellIdentifier, for: indexPath) as! ConfirmOrderGeneralInfoCell
        cell.selectionStyle = .none
        let value = confirmationModel.formattedValue(for: item)
        cell.update(with: item, value: value)

        cell.infoHandle = { [weak self] in
            self?.feeInfoAction()
        }
        return cell
    }
}
