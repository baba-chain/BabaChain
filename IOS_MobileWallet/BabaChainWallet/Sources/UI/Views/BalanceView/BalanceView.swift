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

// MARK: - BalanceViewDataSource

@objc(DWBalanceViewDataSource)
protocol BalanceViewDataSource: AnyObject {
    var mainAmountString: String { get }
    var supplementaryAmountString: String { get }
}

// MARK: - BalanceView

final class BalanceView: UIView {
    public weak var dataSource: BalanceViewDataSource? {
        didSet {
            reloadView()
        }
    }

    public var babachainSymbolColor: UIColor? {
        didSet {
            reloadView()
        }
    }

    public var tint: UIColor? {
        didSet {
            reloadView()
        }
    }

    override var intrinsicContentSize: CGSize {
        CGSize(width: BalanceView.noIntrinsicMetric, height: 52.0)
    }

    private var container: UIStackView!
    private var babachainBalanceLabel: UILabel!
    private var fiatBalanceLabel: UILabel!

    override init(frame: CGRect) {
        super.init(frame: frame)

        configureHierarchy()
    }

    required init?(coder: NSCoder) {
        super.init(coder: coder)

        configureHierarchy()
    }
}

extension BalanceView {
    public func reloadData() {
        reloadView()
    }

    private func reloadView() {
        guard let babachainBalanceLabel,
              let fiatBalanceLabel else { return }
        
        let mainAmountString = dataSource?.mainAmountString ?? NumberFormatter.babachainFormatter.string(from: 0)!
        let supplementaryAmountString = dataSource?.supplementaryAmountString ?? NumberFormatter.fiatFormatter.string(from: 0)!

        let balanceColor = UIColor.label
        let balanceString = mainAmountString.attributedAmountStringWithBabaChainSymbol(tintColor: tint ?? balanceColor)
        babachainBalanceLabel.attributedText = balanceString
        fiatBalanceLabel.text = supplementaryAmountString
        fiatBalanceLabel.textColor = tint ?? UIColor.dw_secondaryText()
    }

    private func configureHierarchy() {
        backgroundColor = .clear

        container = UIStackView()
        container.translatesAutoresizingMaskIntoConstraints = false
        container.axis = .vertical
        addSubview(container)

        babachainBalanceLabel = UILabel()
        babachainBalanceLabel.translatesAutoresizingMaskIntoConstraints = false
        babachainBalanceLabel.font = .dw_font(forTextStyle: .title1)
        babachainBalanceLabel.textAlignment = .center
        container.addArrangedSubview(babachainBalanceLabel)

        fiatBalanceLabel = UILabel()
        fiatBalanceLabel.translatesAutoresizingMaskIntoConstraints = false
        fiatBalanceLabel.font = .dw_font(forTextStyle: .callout)
        fiatBalanceLabel.textAlignment = .center
        container.addArrangedSubview(fiatBalanceLabel)

        NSLayoutConstraint.activate([
            container.topAnchor.constraint(equalTo: topAnchor),
            container.bottomAnchor.constraint(equalTo: bottomAnchor),
            container.leadingAnchor.constraint(equalTo: leadingAnchor),
            container.trailingAnchor.constraint(equalTo: trailingAnchor),
        ])

        reloadView()
    }
}
